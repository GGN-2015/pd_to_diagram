from typing import Dict, Any, List

def get_int_list_input_from_dict(solution: Dict[str,Any]) -> List[int]:
    arr = []

    arr.append(solution['grid_size'])       # 网格大小
    arr.append(solution['crossing_number']) # 交叉点数目

    for i in range(solution['crossing_number']):  # 依次输出：位置，朝向，pd_code
        arr += solution["pos_list"][i]
        arr.append(solution['direction_list'][i])
        arr += solution["pd_code"][i]
    return arr

if __name__ == "__main__":
    # solution = {'grid_size': 111, 'crossing_number': 11, 'pos_list': [[76, 81], [53, 100], [63, 54], [55, 19], [34, 32], [46, 68], [41, 41], [58, 28], [59, 51], [82, 89], [53, 92]], 'direction_list': [1, 0, 1, 2, 1, 3, 3, 1, 0, 3, 2], 'pd_code': [[6, 1, 7, 2], [3, 13, 4, 12], [7, 21, 8, 20], [19, 5, 20, 10], [13, 19, 14, 22], [21, 11, 22, 18], [17, 15, 18, 14], [9, 17, 10, 16], [15, 9, 16, 8], [2, 5, 3, 6], [11, 1, 12, 4]]}
    solution = {'grid_size': 111, 'crossing_number': 11, 'pos_list': [[76, 81], [37, 87], [63, 54], [55, 19], [34, 32], [62, 61], [48, 44], [49, 37], [59, 51], [85, 84], [47, 85]], 'direction_list': [1, 1, 1, 2, 1, 3, 0, 1, 0, 2, 2], 'pd_code': [[6, 1, 7, 2], [3, 13, 4, 12], [7, 21, 8, 20], [19, 5, 20, 10], [13, 19, 14, 22], [21, 11, 22, 18], [17, 15, 18, 14], [9, 17, 10, 16], [15, 9, 16, 8], [2, 5, 3, 6], [11, 1, 12, 4]]}
    print(" ".join(list(map(str, get_int_list_input_from_dict(solution)))))
