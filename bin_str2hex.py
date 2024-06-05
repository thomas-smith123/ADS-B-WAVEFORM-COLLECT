import binascii 
def bin_str_matrix_to_hex(matrix):
    hex_matrix = []
    for i in range(0, len(matrix), 4):
        binary_chunk = matrix[i:i+4]
        tmp_ = ''.join(binary_chunk)
        hexadecimal_string = hex(int(tmp_, 2))[2:].upper()
        hex_matrix.append(hexadecimal_string)

    return ''.join(hex_matrix)
