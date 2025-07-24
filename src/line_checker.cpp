// 给定一个网格图，上面放置了若干个扭结交叉点
// 我们需要试图使用最短路原则，将这些交叉点连接起来
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <queue>
#include <string>
#include <tuple>
#include <vector>
#include <map>

// #include <cassert>
// 减小错误出现时候的时间开销，后续我们会判断返回值非零
#define assert(X) if(!(X)) { printf(#X " = %d", (X)); while(1) ; }

// 设置为 0 表示不要输出辅助调试阶段信息，设置为 1 表示需要输出
#define DEBUG_OUTPUT (0)

// 在初始化阶段，计算一下根号 2 等于多少
const double SQRT_2 = sqrt(2);

// 默认的方向顺序是：东、北、西、南
// delta 的值会影响方向的顺序
const int DIR_DX[] = {+1,  0, -1,  0};
const int DIR_DY[] = { 0, +1,  0, -1};

// 四个角落上的方向：东北、西北、西南、东南
// 采用逆时针枚举顺序
const int CORNER_DX[] = {+1, -1, -1, +1};
const int CORNER_DY[] = {+1, +1, -1, -1};

typedef std::vector<int> IntList;
typedef std::vector<std::tuple<int,int>> IntPairList;
typedef std::vector<std::tuple<int,int,int,int>> PdCode;

// 描述算法的输入
class AlgorithmInput {
private:
    int grid_size, crossing_number, bad_pair;
    std::string status = "unsolve"; // 总共有三种状态：unsolve, fail, success
    double answer_length = 0;       // 只有在 sucess 状态下，才有意义，fail 说明结果是 inf

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

    // 记录交叉点之间的连接关系
    // 例如一条路经 x1 -> x2 -> ... -> xn
    // 那么：
    // chain_prev[x1] =     x1, chain_next[x1] = x2（起点的上一个点是自己）
    // chain_prev[x2] =     x1, chain_next[x2] = x3
    // ...
    // chain_prev[xn] = x{n-1}, chain_next[xn] = xn（终点的下一个点是自己）
    // 这样我们可以通过两个方向遍历整条链
    // 而谁是起点谁是终点，取决于数组 socket_position 中两次出现之间的顺序
    std::map<std::tuple<int, int>, std::tuple<int, int>> chain_next;
    std::map<std::tuple<int, int>, std::tuple<int, int>> chain_prev;

    void clear() {
        grid_size = crossing_number = bad_pair = 0;
        status = "unsolve";
        pos_list.clear();
        direction_list.clear();
        pd_code.clear();
        count_number.clear();
        socket_position.clear();
        chess_board.clear();
        distance_rank.clear();
        chain_next.clear();
        chain_prev.clear();
    }

    // 检查 pos1 和 pos2 是否具有边连接关系
    // 在对角线是否可走的判断中，我们需要判断另一条对偶的对角线是否被占用且具有连边关系
    bool check_is_linked(std::tuple<int, int> pos1, std::tuple<int, int> pos2) const {

        // 以下两个 return 的情况，说明两个点中有至少一个没有被占用
        if(chain_next.find(pos1) == chain_next.end()) return false;
        if(chain_next.find(pos2) == chain_next.end()) return false; 
        
        return 
            chain_next.find(pos1) -> second == pos2 || // 四个方向都需要检查
            chain_next.find(pos2) -> second == pos1 ||
            chain_prev.find(pos1) -> second == pos2 ||
            chain_prev.find(pos2) -> second == pos1;
    }

    static double sqr(double x) { // 仅仅是算个平方
        return x * x;
    }
public:
    double getAnswerLength() const {
        assert(status != "unsolve");
        return answer_length + bad_pair * sqr(grid_size - 1) * 2.0; // 对 bap pair 引入了很大的惩罚
    }

    static double distance_estimate(int dx, int dy) { // 用于实现启发式估价函数
        dx = std::abs(dx);
        dy = std::abs(dy);
        return sqrt(2) * std::min(dx, dy) + (std::max(dx, dy) - std::min(dx, dy)); // 一个距离估计
    }
    static double distance_estimate(std::tuple<int, int> pos1, std::tuple<int, int> pos2) { // 重载
        auto [x1, y1] = pos1;
        auto [x2, y2] = pos2;
        return distance_estimate(x1 - x2, y1 - y2);
    }
    double get_distance_between_socket(int tmp) const {
        assert(socket_position.find(tmp) != socket_position.end());
        assert(socket_position.find(tmp) -> second.size() == 2);
        auto pos1 = socket_position.find(tmp) -> second[0];
        auto pos2 = socket_position.find(tmp) -> second[1];
        return distance_estimate(pos1, pos2);
    }
    // 从文件或键盘输入一个交叉点布局
    void inputFromFpin(bool quiet=false, FILE* fpin=stdin) {
        status = "unsolve"; // 每次输入新的数据时，需要将所有数据结构清空
        clear();

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

        std::sort(distance_rank.begin(), distance_rank.end()); // 按照字典序排序，这样能够优先处理距离远的，我觉得离得近的更不容易被破坏
        std::reverse(distance_rank.begin(), distance_rank.end());
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
        int xmin, xmax, ymin, ymax; // 框选一个区域显示
        xmin = grid_size + 1;
        xmax = 0;
        ymin = grid_size + 1;
        ymax = 0;
        for(int j = grid_size; j > 0; j -= 1) {
            for(int i = grid_size; i > 0; i -= 1) {
                if(chess_board.count(std::make_tuple(i, j)) > 0 && chess_board.find(std::make_tuple(i, j))->second > 0) {
                    xmin = std::min(xmin, i);
                    xmax = std::max(xmax, i);
                    ymin = std::min(ymin, j);
                    ymax = std::max(ymax, j);
                }
            }
        }
        for(int j = ymax; j >= ymin; j -= 1) {
            fprintf(fpout, "diag: (y = %4d) ", j); // 输出行号
            for(int i = xmin; i <= xmax; i += 1) {
                if(chess_board.count(std::make_tuple(i, j)) <= 0) {
                    fprintf(fpout, "   0");
                }else {
                    fprintf(fpout, "%4d", chess_board.find(std::make_tuple(i, j)) -> second);
                }
            }
            fprintf(fpout, "\n");
        }
    }

    // A* 算法中使用优先队列维护的数据结构
    struct AStarNode {
        double g; // 目前在路上已经花费的成本
        double h; // 当前位置距离终点的股价距离

        double f() const { // 用于参与比较的综合估价函数
            return g + h;
        }
        std::tuple<int, int> pos_now;    // 当前位置

        void* astar_node_handle_container; // 记录父节点，实际类型是 std::vector<AStarNode>*
        int astar_node_handle_index;       // 由于类的顺序，我们这里勉强先存一个 void*
    };
    typedef std::tuple<std::vector<AStarNode>*, int> AStarNodeHandle;

    struct AStarNodeCmp { // 维护小根堆的比较结构体
        bool operator()(AStarNodeHandle asnh1, AStarNodeHandle asnh2) {
            auto asnh_c1 = std::get<0>(asnh1);
            auto asnh_i1 = std::get<1>(asnh1);
            auto asnh_c2 = std::get<0>(asnh2);
            auto asnh_i2 = std::get<1>(asnh2);
            return (*asnh_c1)[asnh_i1].f() > (*asnh_c2)[asnh_i2].f();
        }
    };
    typedef std::priority_queue<AStarNodeHandle, std::vector<AStarNodeHandle>, AStarNodeCmp> AStarHeap; // 小根堆，先弹出的总是综合估价较小的

    // 在容器中申请一个新节点（在容器中申请是为了方便将来回收空间）
    AStarNodeHandle create_new_node(std::vector<AStarNode>& container, double g, double h, std::tuple<int, int> pos_now, AStarNodeHandle prev_astar_node) {
        container.push_back((AStarNode){
            .g=g,
            .h=h,
            .pos_now=pos_now,
            .astar_node_handle_container=std::get<0>(prev_astar_node),
            .astar_node_handle_index=std::get<1>(prev_astar_node)
        });
        return std::make_tuple(&container, container.size() - 1);
    }

    // 已知两个节点构成对角，计算另外两个节点的坐标
    static std::tuple<std::tuple<int, int>, std::tuple<int, int>> get_other_corners(int x1, int y1, int x2, int y2) {
        assert(std::abs(x1 - x2) == 1);
        assert(std::abs(y1 - y2) == 1);
        auto ans = std::make_tuple(std::make_tuple(x1, y2), std::tuple(x2, y1));
        return ans;
    }

    static AStarNodeHandle get_prev_node(AStarNodeHandle node_now) { // 获取后继节点
        assert(std::get<1>(node_now) != -1);
        auto real_node = ((*std::get<0>(node_now))[std::get<1>(node_now)]);
        return std::make_tuple(std::get<0>(node_now), real_node.astar_node_handle_index);
    }
    static std::tuple<int, int> get_pos_now(AStarNodeHandle node_now) {
        assert(std::get<1>(node_now) != -1);
        auto real_node = ((*std::get<0>(node_now))[std::get<1>(node_now)]);
        return real_node.pos_now;
    }
    static AStarNode get_top_node(AStarNodeHandle top_node_handle) {
        return (*std::get<0>(top_node_handle))[std::get<1>(top_node_handle)];
    }

    // 指定一个 socket 编号
    // 在当前局面下试图将两个编号连起来
    double create_path_for_socket(int socket_index) {
        assert(socket_position[socket_index].size() == 2); // 必须恰好有两个插座才能使用
        auto begin_pos = socket_position[socket_index][0];
        auto end_pos   = socket_position[socket_index][1]; // 记录起始位置和终止位置


        // 创建树节点容器
        std::vector<AStarNode> container;
        AStarNodeHandle nullptr_handle = std::make_tuple(&container, -1);
        std::map<std::tuple<int, int>, double> minG; // 记录每个访问过的节点的最近的 G 值

        // 搜索树的根节点
        AStarNodeHandle root = create_new_node(
            container, 0, distance_estimate(begin_pos, end_pos), begin_pos, nullptr_handle);

        // 最开始的时候堆中只有一个元素
        AStarHeap astar_heap; astar_heap.push(root);
        
        // 记录搜索树中最优的路线对应的结点
        AStarNodeHandle best_route = nullptr_handle;
        
        // 每次从堆中弹一个元素出来
        if(DEBUG_OUTPUT) printf(" - begin astar\n"); fflush(stdout);
        while(astar_heap.size() > 0) {
            if(DEBUG_OUTPUT) printf(" - astar_heap.size() = %d\n", (int)astar_heap.size()); fflush(stdout);
            auto top_node_handle = astar_heap.top(); astar_heap.pop();

            if(minG.count(get_top_node(top_node_handle).pos_now) <= 0) { // 这说明，我们第一次访问这个节点
                minG[get_top_node(top_node_handle).pos_now] = std::numeric_limits<double>::infinity();
            }

            // 如果之前访问时，使用过的距离小于等于这一次访问时已经走的距离，则没有必要对这一节点进行进一步拓展
            if(minG[get_top_node(top_node_handle).pos_now] <= get_top_node(top_node_handle).g) continue;

            // 更新最优解
            minG[get_top_node(top_node_handle).pos_now] = get_top_node(top_node_handle).g;
            if(get_top_node(top_node_handle).pos_now == end_pos) { // 说明找到了最短路
                best_route = top_node_handle;
                break;
            }

            if(DEBUG_OUTPUT) printf(" - considering four sides\n"); fflush(stdout);

            // 如果程序成功运行到这里，说明需要对当前节点进行拓展
            // 我们先考虑向水平竖直的四个方向进行拓展，然后再考虑向八个对角进行拓展，一定要注意障碍物相关的问题
            auto [xnow, ynow] = get_top_node(top_node_handle).pos_now;
            for(int d = 0; d <= 3; d += 1) {
                int xnxt = xnow + DIR_DX[d];
                int ynxt = ynow + DIR_DY[d];
                auto pos_nxt = std::make_tuple(xnxt, ynxt); //这是下一步要走到的位置
                double gnxt  = get_top_node(top_node_handle).g + 1;             // 因为多走了一步
                if(!(1 <= xnxt && xnxt <= grid_size && 1 <= ynxt && ynxt <= grid_size)) { // 超出地图外了，不可以走
                    continue;
                }
                if(chess_board[pos_nxt] != 0 && chess_board[pos_nxt] != socket_index) { // 说明下一个位置有障碍物，不可以走
                    continue;
                }

                // 程序执行到这里，说明从起始位置出发，存在一条抵达 pos_nxt 的路径
                // 送入小根堆
                astar_heap.push(create_new_node(container,
                    gnxt, distance_estimate(pos_nxt, end_pos), pos_nxt, top_node_handle));
            }

            if(DEBUG_OUTPUT) printf(" - considering four corners\n"); fflush(stdout);

            // 现在再考虑四个角落上的方向
            for(int d = 0; d <= 3; d += 1) {
                if(DEBUG_OUTPUT) printf(" - corner %d phase 0\n", d); fflush(stdout);

                int xnxt = xnow + CORNER_DX[d];
                int ynxt = ynow + CORNER_DY[d];
                auto pos_nxt = std::make_tuple(xnxt, ynxt); //这是下一步要走到的位置
                double gnxt  = get_top_node(top_node_handle).g + SQRT_2;        // 因为多走了一步

                if(DEBUG_OUTPUT) printf(" - corner %d phase 1\n", d); fflush(stdout);

                if(!(1 <= xnxt && xnxt <= grid_size && 1 <= ynxt && ynxt <= grid_size)) { // 超出地图外了，不可以走
                    continue;
                }
                if(chess_board[pos_nxt] != 0 && chess_board[pos_nxt] != socket_index) { // 说明下一个位置有障碍物，不可以走
                    continue;
                }

                if(DEBUG_OUTPUT) printf(" - corner %d phase 2\n", d); fflush(stdout);

                // 四个角点需要额外考虑对偶对角是否被占据的情况
                // 如果另外两个对角恰好有边，那么就不能这么斜着走过去
                auto [cor1, cor2] = get_other_corners(xnow, ynow, xnxt, ynxt);
                if(chess_board[cor1] != 0 && chess_board[cor1] == chess_board[cor2] && check_is_linked(cor1, cor2)) {
                    continue;
                }

                if(DEBUG_OUTPUT) printf(" - corner %d phase 3\n", d); fflush(stdout);

                // 程序执行到这里，说明从起始位置出发，存在一条抵达 pos_nxt 的路径
                // 送入小根堆
                astar_heap.push(create_new_node(container,
                    gnxt, distance_estimate(pos_nxt, end_pos), pos_nxt, top_node_handle));

                if(DEBUG_OUTPUT) printf(" - corner %d done\n", d); fflush(stdout);
            }

            if(DEBUG_OUTPUT) printf(" - next pos analized\n"); fflush(stdout);
        }
        if(DEBUG_OUTPUT) printf(" - end astar\n"); fflush(stdout);

        // 执行到这里如果 best_route 仍没有被赋值，这说明目标位置不可达
        if(best_route == nullptr_handle) {
            return std::numeric_limits<double>::infinity();
        }

        // 否则说明程序找到了最优路线
        auto node_now = best_route;
        while(node_now != nullptr_handle && (std::get<1>(node_now) != -1) && node_now != root) { // 维护双向链
            auto node_prev = get_prev_node(node_now);
            assert(get_pos_now(node_prev) != get_pos_now(node_now));

            chain_next[get_pos_now(node_prev)] = get_pos_now(node_now );
            chain_prev[get_pos_now(node_now )] = get_pos_now(node_prev);

            // 进一步确保走过的路线全是安全的
            assert(chess_board[get_pos_now(node_now)] == 0 || chess_board[get_pos_now(node_now)] == socket_index);
            chess_board[get_pos_now(node_now)] = socket_index;

            node_now = node_prev; // 记得前进
        }
        chain_prev[begin_pos] = begin_pos; // 保证路径上的每一个点都有前驱和后继
        chain_next[end_pos]   = end_pos;

        // 返回路线总长度
        return minG[end_pos];
    }

    // 算法核心：计算每一对 socket 之间的连接方式
    // 函数的返回值是所有连线的总长度
    void solveAll() {
        assert(status == "unsolve");

        double total_len = 0;
        for(auto [d, v]: distance_rank) {
            if(DEBUG_OUTPUT) printf("creating path for %d\n", v); fflush(stdout);

            double tmp;
            tmp = create_path_for_socket(v); // 按照从近到远的顺序依次为所有 socket 对构建连线
            if(std::isinf(tmp)) {
                bad_pair += 1;
            }else {
                total_len += tmp;
            }
        }
        answer_length = total_len;

        if(bad_pair != 0) { // 有无法连接的 socket 共计 bad_pair 个
            status = "fail";
        }else {
            status = "success";
        }
    }

    // 输出节点之间的连接关系
    void outputChainMap() {
        if(status == "unsolve") { // 调用时自动求解即可
            solveAll();
        }
        printf("length: %.15f\n", getAnswerLength()); // 输出总布线长度，如果方案不合法输出 inf
        for(auto [pos_now, pos_nxt]: chain_next) {
            auto [x1, y1] = pos_now;
            auto [x2, y2] = pos_nxt;
            if(pos_now != pos_nxt) {
                printf("link: %d %d and %d %d\n", x1, y1, x2, y2);
            }
        }
        printf("json: %s\n", serialize().c_str()); // 输出一个 json 字符串版本
    }
};

// 程序使用方式
// 1. ./line_checker.out 直接使用：这样的话会启用 stdin 并交互式输入数据
// 2. ./line_checker.out "文件路径"：从文件路径中读取出数据，不输出交互信息
extern "C" double call_main(char* filename) {
    FILE* fpin = fopen(filename, "r"); // 试图打开文件
    bool quiet = true;
    if(fpin == nullptr) { // 打开文件失败
        exit(1);
    }
    AlgorithmInput algo_input;
    algo_input.inputFromFpin(quiet, fpin);
    algo_input.solveAll();
    return algo_input.getAnswerLength();
}

int main(int argc, char** argv) {
    FILE* fpin = stdin;
    bool quiet = false;
    if(argc == 2) { // 从文件获取
        return call_main(argv[1]);
    }else {
         AlgorithmInput algo_input; // 从标准输入获取
        algo_input.inputFromFpin(quiet, fpin);
        algo_input.outputChainMap();
        algo_input.debugShowChessBoard();
    }
    return 0;
}