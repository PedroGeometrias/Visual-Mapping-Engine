width = 10
height = 10

with open("assets/checkerboard.ppm", "wb") as f:
    # P6 header
    f.write(b"P6\n")
    f.write(f"{width} {height}\n".encode())
    f.write(b"255\n")

    # Pixel data: alternating white and black pixels
    for y in range(height):
        for x in range(width):
            if (x + y) % 2 == 0:
                f.write(bytes([255, 255, 255]))  # White
            else:
                f.write(bytes([0, 0, 0]))        # Black

