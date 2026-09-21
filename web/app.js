(async function(){
    const dropZone = document.getElementById("drop-zone");
    const buildButton = document.getElementById("build");
    const downloadButton = document.getElementById("download");
    const clearButton = document.getElementById("clear");
    const status = document.getElementById("status");
    const fileList = document.getElementById("file-list");
    const canvas = document.getElementById("panorama");
    const emptyOutput = document.getElementById("empty-output");

    const MAX_PREVIEW_PIXELS = 8_000_000;
    const MAX_PREVIEW_DIMENSION = 4096;

    let files = [];
    let engine;
    let hasPanorama = false;

    function setStatus(message, error = false){
        status.textContent = message;
        status.classList.toggle("error", error);
    }

    function refreshControls(){
        buildButton.disabled = !engine || files.length < 2;
        downloadButton.disabled = !engine || !hasPanorama;
        clearButton.disabled = files.length === 0 && !hasPanorama;
    }

    function refreshFileList(){
        fileList.replaceChildren();

        for(const file of files){
            const item = document.createElement("li");
            item.textContent = file.name;
            fileList.appendChild(item);
        }
    }

    function resetOutput(message = "The panorama preview will appear here when it is small enough."){
        canvas.width = 0;
        canvas.height = 0;
        canvas.style.display = "none";
        emptyOutput.textContent = message;
        emptyOutput.style.display = "block";
        hasPanorama = false;
    }

    function wasmError(){
        return engine.ccall("vme_last_error", "string", [], []) || "Unknown engine error.";
    }

    function isImage(file){
        return file.type.startsWith("image/") || /\.(jpe?g|png|bmp|ppm)$/i.test(file.name);
    }

    function addDroppedFiles(droppedFiles){
        const images = Array.from(droppedFiles).filter(isImage);

        if(images.length === 0){
            setStatus("No supported image files were dropped.", true);
            return;
        }

        const existing = new Set(files.map(file => `${file.name}:${file.size}:${file.lastModified}`));

        for(const file of images){
            const key = `${file.name}:${file.size}:${file.lastModified}`;
            if(!existing.has(key)){
                files.push(file);
                existing.add(key);
            }
        }

        if(engine){
            engine.ccall("vme_reset", null, [], []);
        }

        resetOutput();
        refreshFileList();
        setStatus(files.length < 2 ? "Drop at least one more overlapping image." : `${files.length} images ready.`);
        refreshControls();
    }

    async function addImage(file){
        const bytes = new Uint8Array(await file.arrayBuffer());
        const pointer = engine._malloc(bytes.length);

        try{
            engine.HEAPU8.set(bytes, pointer);
            return engine.ccall(
                "vme_add_image",
                "number",
                ["number", "number"],
                [pointer, bytes.length]
            ) !== 0;
        }finally{
            engine._free(pointer);
        }
    }

    function panoramaInfo(){
        const width = engine.ccall("vme_panorama_width", "number", [], []);
        const height = engine.ccall("vme_panorama_height", "number", [], []);
        const size = engine.ccall("vme_panorama_size", "number", [], []);
        const pointer = engine.ccall("vme_panorama_pixels", "number", [], []);

        if(width <= 0 || height <= 0 || size !== width * height * 3 || !pointer){
            throw new Error("The engine returned an invalid panorama image.");
        }

        return { width, height, size, pointer };
    }

    function canPreview(width, height){
        return width <= MAX_PREVIEW_DIMENSION &&
               height <= MAX_PREVIEW_DIMENSION &&
               width * height <= MAX_PREVIEW_PIXELS;
    }

    function drawPanorama(info){
        const rgb = engine.HEAPU8.slice(info.pointer, info.pointer + info.size);
        const rgba = new Uint8ClampedArray(info.width * info.height * 4);

        for(let src = 0, dst = 0; src < rgb.length; src += 3, dst += 4){
            rgba[dst] = rgb[src];
            rgba[dst + 1] = rgb[src + 1];
            rgba[dst + 2] = rgb[src + 2];
            rgba[dst + 3] = 255;
        }

        canvas.width = info.width;
        canvas.height = info.height;
        canvas.getContext("2d").putImageData(new ImageData(rgba, info.width, info.height), 0, 0);
        canvas.style.display = "block";
        emptyOutput.style.display = "none";
    }

    function downloadPanorama(){
        const info = panoramaInfo();
        const header = new TextEncoder().encode(`P6\n${info.width} ${info.height}\n255\n`);
        const rgb = engine.HEAPU8.slice(info.pointer, info.pointer + info.size);
        const blob = new Blob([header, rgb], { type: "image/x-portable-pixmap" });
        const url = URL.createObjectURL(blob);
        const link = document.createElement("a");

        link.href = url;
        link.download = "panorama.ppm";
        document.body.appendChild(link);
        link.click();
        link.remove();
        URL.revokeObjectURL(url);
    }

    document.addEventListener("dragenter", function(event){
        event.preventDefault();
        if(Array.from(event.dataTransfer.types).includes("Files")){
            dropZone.classList.add("dragging");
        }
    });

    document.addEventListener("dragover", function(event){
        event.preventDefault();
    });

    document.addEventListener("dragleave", function(event){
        if(!event.relatedTarget){
            dropZone.classList.remove("dragging");
        }
    });

    document.addEventListener("drop", function(event){
        event.preventDefault();
        dropZone.classList.remove("dragging");
        addDroppedFiles(event.dataTransfer.files);
    });

    clearButton.addEventListener("click", function(){
        files = [];
        engine.ccall("vme_reset", null, [], []);
        refreshFileList();
        resetOutput();
        setStatus("Drop at least two overlapping images.");
        refreshControls();
    });

    downloadButton.addEventListener("click", function(){
        try{
            downloadPanorama();
        }catch(error){
            setStatus(error.message, true);
        }
    });

    buildButton.addEventListener("click", async function(){
        buildButton.disabled = true;
        downloadButton.disabled = true;
        setStatus("Running C++ panorama pipeline…");
        resetOutput();
        engine.ccall("vme_reset", null, [], []);

        try{
            for(const file of files){
                if(!await addImage(file)){
                    throw new Error(wasmError());
                }
            }

            const ok = engine.ccall("vme_build_panorama", "number", [], []) !== 0;

            if(!ok){
                throw new Error(wasmError());
            }

            const info = panoramaInfo();
            hasPanorama = true;

            if(canPreview(info.width, info.height)){
                try{
                    drawPanorama(info);
                    setStatus(`Panorama complete (${info.width} × ${info.height}).`);
                }catch(error){
                    resetOutput(`Panorama built (${info.width} × ${info.height}), but the browser could not display the preview. Download is still available.`);
                    hasPanorama = true;
                    setStatus("Panorama complete. Preview unavailable; download is ready.");
                }
            }else{
                resetOutput(`Panorama built (${info.width} × ${info.height}). Preview skipped because the result is too large; download is still available.`);
                hasPanorama = true;
                setStatus("Panorama complete. Preview skipped; download is ready.");
            }
        }catch(error){
            hasPanorama = false;
            setStatus(error.message, true);
        }finally{
            refreshControls();
        }
    });

    try{
        engine = await VisualMappingEngine();
        setStatus("Drop at least two overlapping images.");
    }catch(error){
        setStatus(`Could not load WebAssembly: ${error.message}`, true);
    }

    refreshControls();
})();
