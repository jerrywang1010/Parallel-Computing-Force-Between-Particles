def read_file(filename):
    data = {}
    with open(filename, 'r') as f:
        for line in f:
            if "ID" in line:
                parts = line.strip().split(',')
                id_part, force_part = parts
                id_val = int(id_part.split(':')[1].strip())
                force_val = float(force_part.split('=')[1].strip())
                data[id_val] = force_val
    return data

def compare_files(file1, file2, threshold=1e-10):
    data1 = read_file(file1)
    data2 = read_file(file2)

    # Check if both files have the same IDs
    if set(data1.keys()) != set(data2.keys()):
        print("The files have different IDs!")
        return

    for id_val, force_val1 in data1.items():
        force_val2 = data2[id_val]
        if abs(force_val1 - force_val2) > threshold:
            print(f"ID: {id_val} has different forces. File1: {force_val1}, File2: {force_val2}")

    print(f"all correct!")

if __name__ == "__main__":
    import sys

    if len(sys.argv) != 3:
        print("Usage: python compare.py <output_file1> <output_file2>")
        sys.exit(1)

    file1 = sys.argv[1]
    file2 = sys.argv[2]

    compare_files(file1, file2)
