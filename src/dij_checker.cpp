// 给定一个网格图，上面放置了若干个扭结交叉点
// 我们需要试图使用最短路原则，将这些交叉点连接起来
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <tuple>
#include <string>
#include <vector>
#include <map>

// 默认的方向顺序是：东、北、西、南
// delta 的值会影响方向的顺序
const int DIR_DX[] = {+1,  0, -1,  0, 0};
const int DIR_DY[] = { 0, +1,  0, -1, 0};

typedef std::vector<int> IntList;
typedef std::vector<std::tuple<int,int>> IntPairList;
typedef std::vector<std::tuple<int,int,int,int>> PdCode;

// 描述算法的输入
class AlgorithmInput {
private:
    int grid_size, crossing_number;

    // 三个数组：pos_list、direction_list、pd_code 具有相同的长度，同一个下标描述同一个交叉点
    IntPairList pos_list;   // 记录每个交叉点所在的位置
    IntList direction_list; // 记录每个交叉点的朝向
    PdCode pd_code;         // 记录原始 pd_code

    // 两个计数数组，用于统计弧线编号以及位置的占据情况，用于冲突检测
    std::map<int, int> count_number;
    std::map<int, IntPairList> socket_position; // 每个插头编号会出现两次，记录出现位置
    std::map<std::tuple<int, int>, int> chess_board; // 记录棋盘现状，就是每个位置有什么插头

    // 用于根据欧氏距离对所有具有相同编号的插头对，进行排序
    std::vector<std::tuple<double, int>> distance_rank;

    static double sqr(double x) { // 仅仅是算个平方
        return x * x;
    }
public:
    static double distance_estimate(int dx, int dy) {
        dx = std::abs(dx);
        dy = std::abs(dy);
        return sqrt(2) * std::min(dx, dy) + (std::max(dx, dy) - std::min(dx, dy)); // 一个距离估计
    }
    double get_distance_between_socket(int tmp) const {
        assert(socket_position.find(tmp) != socket_position.end());
        assert(socket_position.find(tmp) -> second.size() == 2);
        auto [x1, y1] = socket_position.find(tmp) -> second[0];
        auto [x2, y2] = socket_position.find(tmp) -> second[1];
        auto dx = x1 - x2;
        auto dy = y1 - y2;
        return distance_estimate(dx, dy);
    }
    // 从文件或键盘输入一个交叉点布局
    void inputFromFpin(bool quiet=false, FILE* fpin=stdin) {

        if(!quiet) printf("grid_size:");
        fscanf(fpin, "%d", &grid_size); // 输入网格图的大小，网格图的尺寸为 grid_size * grid_size

        if(!quiet) printf("crossing_number:");
        fscanf(fpin, "%d", &crossing_number); // 输入交叉点的数目

        // 输入每个交叉点的：位置，朝向，以及 PD_CODE
        for(int i = 1; i <= crossing_number; i += 1) {

            if(!quiet) printf("node[%d] posx, posy:", i);
            int posx, posy; fscanf(fpin, "%d%d", &posx, &posy);
            assert(1 <= posx - 1 && posx + 1 <= grid_size); // 地图的坐标为 1 ~ grid_size
            assert(1 <= posy - 1 && posy + 1 <= grid_size);
            pos_list.push_back(std::make_tuple(posx, posy));

            if(!quiet) printf("node[%d] direction_delta:", i);
            int direction_delta; fscanf(fpin, "%d", &direction_delta); // 这个系数表示（下方进入弧，位于哪个方向）
            assert(0 <= direction_delta && direction_delta <= 3); // 只有四个方向可以选择
            direction_list.push_back(direction_delta);
           
            // 每个位置只能被恰好占据一次，不可以重叠
            // -1 表示下方进入弧方向位于正东，-2 表示下方进入弧方向位于正北，-3 表示下方进入弧方向位于正西，-4 表示下方进入弧方向位于正南
            chess_board[std::make_tuple(posx, posy)] =  -1 -direction_delta;

            if(!quiet) printf("node[%d] pd_code:", i);
            int pd_code_now[4] = {};
            for(int j = 0; j <= 3; j += 1) { // 输入一个交叉点的 PD_CODE
                int tmp;
                fscanf(fpin, "%d", &tmp);
                pd_code_now[j] = tmp;
                count_number[tmp] += 1; // 统计次数：要求每个弧线恰好出现两次

                // 获取当前插头的坐标
                int pos_now_x = posx + DIR_DX[(j + direction_delta) % 4]; // 前四个方向是可以个在 mod 意义下做加法的
                int pos_now_y = posy + DIR_DY[(j + direction_delta) % 4];

                // 记录当前插头的坐标
                socket_position[tmp].push_back(std::make_tuple(
                    pos_now_x, pos_now_y
                ));
                if(socket_position[tmp].size() == 2) {
                    distance_rank.push_back(std::make_tuple(get_distance_between_socket(tmp), tmp));
                }
                chess_board[std::make_tuple(pos_now_x, pos_now_y)] = tmp; // 在棋盘上记录所有信息
            }
            pd_code.push_back(std::make_tuple(pd_code_now[0], pd_code_now[1], pd_code_now[2], pd_code_now[3]));
        }

        // 检查整数出现次数是否正确：要求整数必须从 1 开始，并且每个弧线编号必须恰好出现两次
        for(int i = 1; i <= 2 * crossing_number; i += 1) {
            assert(count_number[i] == 2);
        }
        assert(chess_board.size() == 5 * crossing_number); // 这意味着所有的交叉点之间没有重叠的可能性
        assert(distance_rank.size() == crossing_number * 2);
        std::sort(distance_rank.begin(), distance_rank.end()); // 按照字典序排序，这样能够优先处理距离近的
    }
    std::string serialize() const { // 将自己翻译为一个 json 对象
        std::string ans = "{";
        ans += "\"grid_size\":" + std::to_string(grid_size) + ",";
        ans += "\"crossing_number\":" + std::to_string(crossing_number) + ",";
        ans += "\"pos_list\":[";
        for(int i = 0; i < pos_list.size(); i += 1) {
            ans += "[" + std::to_string(std::get<0>(pos_list[i])) + "," + std::to_string(std::get<1>(pos_list[i])) + "]";
            if(i != pos_list.size() - 1) {
                ans += ",";
            }
        }
        ans += "],";
        ans += "\"direction_list\":[";
        for(int i = 0; i < direction_list.size(); i += 1) {
            ans += std::to_string(direction_list[i]);
            if(i != pos_list.size() - 1) {
                ans += ",";
            }
        }
        ans += "],";
        ans += "\"pd_code\":[";
        for(int i = 0; i < pos_list.size(); i += 1) {
            auto [a, b, c, d] = pd_code[i];
            ans += "[" + std::to_string(a)+ "," + std::to_string(b) + "," + std::to_string(c) + "," + std::to_string(d) + "]";
            if(i != pos_list.size() - 1) {
                ans += ",";
            }
        }
        ans += "]";
        return ans + "}";
    }
    void debugShowChessBoard(FILE* fpout=stdout) const { // 展示棋盘上现在的所有信息
        for(int j = grid_size; j > 0; j -= 1) {
            for(int i = 1; i <= grid_size; i += 1) {
                if(chess_board.count(std::make_tuple(i, j)) <= 0) {
                    fprintf(fpout, "   0");
                }else {
                    fprintf(fpout, "%4d", chess_board.find(std::make_tuple(i, j)) -> second);
                }
            }
            printf("\n");
        }
        printf("\n");
        for(auto [d, v]: distance_rank) {
            printf("%d: %f\n", v, d);
        }
    }
};

int main() {
    AlgorithmInput algo_input;
    algo_input.inputFromFpin();
    printf("%s\n", algo_input.serialize().c_str());
    algo_input.debugShowChessBoard();
    return 0;
}
