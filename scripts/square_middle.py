width = 64
height = 64

with open("assets/harris_test.ppm", "wb") as f:
    # P6 header
    f.write(b"P6\n")
    f.write(f"{width} {height}\n".encode())
    f.write(b"255\n")

    for y in range(height):
        for x in range(width):
            # White square from (16,16) to (47,47)
            if 16 <= x < 48 and 16 <= y < 48:
                f.write(bytes([255, 255, 255]))
            else:
                f.write(bytes([0, 0, 0]))
