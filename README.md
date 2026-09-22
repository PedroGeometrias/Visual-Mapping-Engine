# Visual Mapping Engine

Creates panoramas from multiple overlapping images, the engine itself is written in C++ and compiled to WebAssembly, so the whole thing can run directly in the browser.

I implemented the CV algos myself, so this project is not me just calling opencv, nothing wrong with that though, but it's more fun to write all the good stuff from time to time

You can try it here:

https://pedrogeometrias.github.io/Visual-Mapping-Engine/

## Why?

I wanted to learn more about CV, and get back into C++, this project was actually really fun, it took me a around a month I think, but now I can truly call this bad boy finished:w

## How does it work?

Very simplified version:

```
Images
  ↓
Grayscale
  ↓
Sobel Gradients
  ↓
Harris Corners
  ↓
128 value descriptors
  ↓
Feature Matching
  ↓
RANSAC + Homography
  ↓
Image Graph
  ↓
Maximum Spanning Tree
  ↓
Global Image Transforms
  ↓
Warp + Blend
  ↓
Panorama
```

### Finding features

First the image is converted to grayscale and I calculate the X and Y gradients using Sobel filters.

Those gradients are then used by a Harris corner detector to find points in the image that should be reasonably easy to recognize again in another image.

I also apply non-maximum suppression after Harris, otherwise one actual corner tends to become a bunch of features extremely close to each other, which isn't particularly useful.

### Describing them

Finding a corner only tells me **where** something interesting is, I still need some way of comparing that corner with features from another image.

For every detected feature I calculate its dominant orientation and build a descriptor around it.

The descriptor uses a 16x16 region split into a 4x4 grid, with an 8-bin orientation histogram for every cell:

```
4 * 4 * 8 = 128
```

so every feature eventually becomes a 128 value vector.

This is inspired by SIFT, but it isn't SIFT. I use Harris for feature detection and there is no scale-space detector here.

### Matching

Descriptors are compared using Euclidean distance.

Just grabbing the closest descriptor creates way too many garbage matches, so there are a couple of checks:

* maximum descriptor distance
* nearest/second-nearest ratio test
* bidirectional matching

The last one basically means:

```
feature A says feature B is its best match
                AND
feature B says feature A is its best match
```

If they disagree, I throw the match away.

Even after all of that, some wrong matches still survive, which is where RANSAC comes in.

## Homographies and RANSAC

The relationship between two overlapping images is represented using a 3x3 homography matrix.

I estimate the homography using normalized DLT.

The points are first normalized, I build the `Ah = 0` system, and Eigen is used to solve it with SVD.

So no, I did not write my own SVD implementation, I do value my sanity at least a little bit.

RANSAC repeatedly grabs four random matches, generates a homography from them and checks how many of the other matches agree with that transformation.

Matches with a reprojection error below the threshold count as inliers.

Bad samples, including degenerate groups with collinear points, get ignored.

Once RANSAC finds the best group of inliers, I calculate the homography again using all of them instead of keeping the matrix made from only four random points.

## Stitching more than two images

This was where the project got more interesting.

I didn't want the program to depend on the user dropping the images in the correct order.

So instead of doing:

```
image 0 -> image 1 -> image 2 -> image 3
```

I compare **every image pair**.

If two images have enough geometrically consistent feature matches to produce a valid homography, they become connected inside an image graph.

Something like:

```
     B
     |
____________
|           |
C           A
|_____|_____|
      |
      D
```

The weight of an edge is mainly based on how many RANSAC inliers that image pair has.

From that graph I build a **maximum spanning tree** using Kruskal's algorithm and union-find.

Basically, I want every image connected while preferring the strongest relationships between them.

Then I choose a reference image and walk through the tree, composing homographies along the way until every image has a transformation into the same coordinate system.

Because of that, the input order doesn't really matter.

This part is funny because I started the project expecting computer vision and somehow ended up writing graph algorithms in the middle of it.

## Creating the panorama

Once every image knows where it belongs, I transform all of their corners to calculate how large the final panorama needs to be.

The actual rendering is done using inverse warping.

Instead of throwing every source pixel forward and hoping it lands nicely somewhere, for every output pixel I ask:

```
where would this pixel have come from in the original image?
```

The resulting coordinate is normally not an integer, so I use bilinear interpolation to sample the source image.

For overlapping regions I use basic feather blending. Pixels close to the edges of an image have less influence than pixels closer to its center.

It is nothing fancy like multiband blending or seam optimization, but it works reasonably well for this project.

After everything is rendered, unused space around the panorama is cropped.

## WebAssembly

The original engine is C++.

I wanted the project to be something you could actually open and play with without compiling anything, so I compiled it to WebAssembly using Emscripten.

The JavaScript side is intentionally pretty dumb.

It basically does:

```
user drops images
        ↓
copy encoded bytes into WASM memory
        ↓
C++ does everything
        ↓
get RGB panorama pixels back
        ↓
display/download result
```

The JS doesn't know anything about Harris, RANSAC, homographies or the image graph.

It only handles the UI and communication with the C++ code.

## About large images

This gets expensive pretty fast.

A 3888x2592 image has a little over 10 million pixels.

Now multiply that by a couple of images and add grayscale copies, gradients, Harris buffers, descriptor data, panorama accumulation buffers, the actual output image and then whatever the browser needs to display it.

The browser starts suffering surprisingly quickly.

Because of that, the web UI has a limit for previewing very large panoramas.

The engine can still finish generating the image and let you download it even if the browser decides displaying the thing is probably a bad idea.

The current download format is binary PPM because it is extremely simple to generate and I already had RGB pixels sitting there anyway.

## The Stack used

### Engine

* C++17
* Eigen
* stb
* Make

### Web

* WebAssembly
* Emscripten
* HTML
* CSS
* JavaScript

No OpenCV.

Eigen is mainly used for matrices and SVD, and stb handles decoding the image files.

The feature detection, descriptors, matching, RANSAC, graph construction, warping and blending are implemented in the project.

## Features

* Harris corner detection
* Sobel gradients
* Rotation-aware 128 value feature descriptors
* Descriptor matching with ratio filtering
* Bidirectional feature matching
* Normalized DLT homography estimation
* RANSAC outlier rejection
* Adaptive RANSAC iteration count
* Pairwise image overlap detection
* Weighted image graph
* Maximum spanning tree using Kruskal
* Union-find
* Automatic reference image selection
* Homography composition through the image graph
* Input images don't need to be ordered
* Inverse image warping
* Bilinear interpolation
* Feather blending
* Automatic panorama cropping
* Browser version using WebAssembly
* Panorama preview and download

## Running it

If you just want to see the thing working, use the web version:

https://pedrogeometrias.github.io/Visual-Mapping-Engine/

If you want to build it yourself, you will need:

* Git
* C++17
* Eigen
* Emscripten
* Python 3

Clone the repo:

```bash
git clone https://github.com/PedroGeometrias/Visual-Mapping-Engine
cd Visual-Mapping-Engine
```

Build the WebAssembly version:

```bash
make
```

Then start the local server:

```bash
make serve
```

and open:

```text
http://localhost:8000
```

Do not open `web/index.html` directly, serve it through HTTP.

## Tests

There are some geometry tests for the homography/RANSAC part.

Run them with:

```bash
make test
```

They test homography recovery for things like translation, rotation, scaling and perspective transformations, and also test RANSAC using a mixture of correct correspondences and random outliers.

## Limitations

This is obviously not Photoshop or a production panorama engine.

Some of the bigger limitations right now are:

* no scale-space feature detector
* no exposure compensation
* no bundle adjustment
* simple feather blending
* no seam optimization
* memory usage gets pretty ridiculous with huge images
* repeated patterns can confuse feature matching
* moving objects are a problem
* large changes in viewpoint can break the homography assumption
* the browser version is doing a lot of work for something running inside a webpage

But for what I wanted from this project, I'm pretty happy with where it ended up.
