import math

print("static const float sin_table[256] = {")
for i in range(256):
    angle = (i / 255.0) * (math.pi / 2.0)
    val = math.sin(angle)
    if i % 8 == 0:
        print("    ", end="")
    print(f"{val:.8f}f", end="")
    if i < 255:
        print(", ", end="")
    if i % 8 == 7:
        print()
print("\n};")