#include <Siv3D.hpp>
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <map>
using namespace std;

const int SIZE = 4;
const int INF = SIZE * SIZE;

const double BoardSize = 500;
const double CellSize = BoardSize / SIZE;

const int BLACK = 1;
const int WHITE = 0;

const int DX[8] = {1, 0, -1, 0, 1, 1, -1, -1};
const int DY[8] = {0, 1, 0, -1, 1, -1, -1, 1};

//map[盤面][cell]=評価値
map<int,map<int,int>> transpos_table;

//デバッグ用
void print(int black,int white){
    vector<string> s(SIZE,"");
    for (int cell = 0; cell < SIZE * SIZE; ++cell) {
        if ((black >> cell) & 1)
            s[cell/SIZE] += "●";
        else if ((white >> cell) & 1)
            s[cell/SIZE] += "○";
        else
            s[cell/SIZE] += ".";
    }
    for(auto i : s){
        cout << i << "\n";
    }
}

//dir方向に移動
int move(int cell, int dir) {
    int x = cell / SIZE + DX[dir];
    int y = cell % SIZE + DY[dir];

    if (x < 0 || x >= SIZE || y < 0 || y >= SIZE)
        return -1;
    else
        return x * SIZE + y;
}

//座標cellが色colか
bool iscolor(int black, int white, int col, int cell) {
    if (cell == -1)
        return false;
    if (col == BLACK)
        return ((black >> cell) & 1);
    else
        return ((white >> cell) & 1);
}

//置けるマスを求める
int put(int black, int white, int col, int cell) {
    if (((black | white) >> cell) & 1) return 0;

    int res = 0;
    for (int dir = 0; dir < 8; ++dir) {
        int rev = 0;

        int cell2 = move(cell, dir);
        while (iscolor(black, white, 1 - col, cell2)) {
            rev |= 1 << cell2;
            cell2 = move(cell2, dir);
        }

        if (iscolor(black, white, col, cell2)) {
            res |= rev;
        }
    }
    return res;
}

//色colの評価値を求める
int count_score(int black, int white, int col) {
    int num_black = 0, num_white = 0, num_empty = 0;
    for (int cell = 0; cell < SIZE * SIZE; ++cell) {
        if ((black >> cell) & 1)
            ++num_black;
        else if ((white >> cell) & 1)
            ++num_white;
        else
            ++num_empty;
    }

    if (num_black > num_white)
        num_black += num_empty;
    else if (num_black < num_white)
        num_white += num_empty;

    if (col == BLACK)
        return num_black - num_white;
    else
        return num_white - num_black;
}

//評価値計算
int dfs(int alpha, int beta, int black, int white, int col) {
    int key = (black << 16) | white;

    vector<int> mine, opp;
    for (int cell = 0; cell < SIZE * SIZE; ++cell) {
        if (put(black, white, col, cell))
            mine.push_back(cell);
        if (put(black, white, 1 - col, cell))
            opp.push_back(cell);
    }

    //終局
    if (mine.empty() && opp.empty()) {
        return count_score(black, white, BLACK);
    }

    //パス
    if (mine.empty()) {
        return -dfs(-beta, -alpha, black, white, 1 - col);
    }

    //評価値
    int res = -INF;

    for (int cell : mine) {
        int rev = put(black, white, col, cell);
        int black2 = black ^ rev;
        int white2 = white ^ rev;

        if (col == BLACK)
            black2 |= 1 << cell;
        else
            white2 |= 1 << cell;

        int score = -dfs(-beta, -alpha, black2, white2, 1 - col);
        transpos_table[key][cell] = score;

        res = max(res, score);
        
        // 枝刈り
        if (res >= beta) return res;

        alpha = max(alpha, res);
    }
    return res;
}

//盤面を描画
void draw_board(int black,int white,const Vec2& pos,const Font& font,int col){
    // 行・列を描画
    for (int32 i = 0; i < SIZE; ++i){
        font(i + 1).draw(15, Arg::center((pos.x - 20), (pos.y + CellSize * i + CellSize / 2)), ColorF(1));
        font(char32(U'a' + i)).draw(15, Arg::center((pos.x + CellSize * i + CellSize / 2), (pos.y - 20 - 2)), ColorF(1));
    }

    Rect(pos.x ,pos.y ,BoardSize, BoardSize).draw(ColorF(0.2,0.7,0.3));
    // グリッドを描画
    for (int32 i = 0; i <= SIZE; ++i){
        Line{ pos.x + CellSize * i, pos.y, pos.x + CellSize * i, pos.y + BoardSize }.draw(3, ColorF(0));
        Line{ pos.x, pos.y + CellSize * i, pos.x + BoardSize, pos.y + CellSize * i }.draw(3, ColorF(0));
    }
    
    for(int i = 0;i < SIZE * SIZE;i++){
        int x = i % SIZE;
        int y = i / SIZE;
        
        int px = pos.x + (x * CellSize) + CellSize / 2;
        int py = pos.y + (y * CellSize) + CellSize / 2;
        
        //石を描画
        if((black >> i) & 1){
            Circle(px,py,CellSize / 2 * 0.8).draw(ColorF(WHITE));
        }
        else if((white >> i) & 1){
            Circle(px,py,CellSize / 2 * 0.8).draw(ColorF(BLACK));
        }
        
        //評価値を描画
        if(put(black,white,col,i) != 0){
            int key = (black << 16) | white;
            font(transpos_table[key][i]).draw(Arg::center((pos.x + CellSize * x + CellSize / 2),  (pos.y + CellSize * y + CellSize / 2)),ColorF(1));
        }
    }
}

//ボードの更新
void board_update(int& black, int& white,vector<int>& mine,vector<int>& opp){
    mine = vector<int>();
    opp = vector<int>();
    for (int cell = 0; cell < SIZE * SIZE; ++cell) {
        if (put(black, white, BLACK, cell)){
            mine.push_back(cell);
        }
        if (put(black, white, WHITE, cell)){
            opp.push_back(cell);
        }
    }
}

//初期化
void init(int& black, int& white,int& col,vector<int>& mine,vector<int>& opp){
    black = 0;
    white = 0;
    black = (1 << 6) | (1 << 9);
    white = (1 << 5) | (1 << 10);
    col = BLACK;
    mine = vector<int>();
    opp = vector<int>();
    board_update(black, white, mine, opp);
}

//自分の手番
void mine_move(int cell,int& black,int& white,int& col,vector<int>& mine,vector<int>& opp){
    cout << "-----BLACK-----" << "\n";
    print(black,white);
    for(auto i : mine){
        cout << i << " ";
    }
    cout << "\n";

    int rev = put(black,white,BLACK,cell);
    black = black ^ rev;
    white = white ^ rev;
    black |= 1 << cell;
    col = 1 - col;

    board_update(black,white,mine,opp);
}

//相手の手番
void opp_move(int& black,int& white,int& col,vector<int>& mine,vector<int>& opp){
    cout << "-----WHITE-----" << "\n";
    print(black,white);
    for(auto i : opp){
        cout << i << " ";
    }
    cout << "\n";
    
    int key = (black << 16) | white;
    int cell = 0;
    int m = -INF;
    for(auto i : opp){
        if(transpos_table[key][i] > m){
            m = transpos_table[key][i];
            cell = i;
        }
    }
    int rev = put(black,white,WHITE,cell);
    black = black ^ rev;
    white = white ^ rev;
    white |= 1 << cell;
    col = 1 - col;
    board_update(black,white,mine,opp);
}

void Main() {
    //背景の色
    Scene::SetBackground(Color( 36, 153, 114 ));
    
    //盤面の左上からの位置
    const Vec2 BoardOffset{ 40, 60 };
    
    //フォント
    const Font font20{ 20 };
    const Font font40{ 40 };
    const Font font50{ 50 };
    
    //石を配置
    int black = (1 << 6) | (1 << 9);
    int white = (1 << 5) | (1 << 10);

    //手番
    int col = BLACK;
    
    //評価値計算
    dfs(-INF, INF, black, white, col);
    
    //打てる手
    vector<int> mine, opp;

    //初期化
    init(black,white,col,mine,opp);
    
    while(System::Update()){
        draw_board(black,white,BoardOffset,font20,col);
        if (SimpleGUI::Button(U"最初から始める", Vec2{ 580, BoardOffset.y })){
            init(black,white,col,mine,opp);
        }
        
        if(mine.empty() && opp.empty()){
            continue;
        }
        
        if(col == BLACK){
            if(mine.empty()){
                col = 1 - col;
                cout << "-----BLACK-----" << "\n";
                cout << "PASS" << "\n";
                continue;
            }
            for(int cell = 0;cell < SIZE * SIZE;cell++){
                int x = cell % SIZE;
                int y = cell / SIZE;
                Rect cell_rect((x * CellSize) + BoardOffset.x,(y * CellSize) + BoardOffset.y,CellSize,CellSize);
                if(cell_rect.leftClicked()){
                    if(put(black, white, BLACK, cell) == 0) continue;
                    mine_move(cell,black,white,col,mine,opp);
                }
            }
        }
        else if(col == WHITE){
            if(opp.empty()){
                col = 1 - col;
                cout << "-----WHITE-----" << "\n";
                cout << "PASS" << "\n";
                continue;
            }
            opp_move(black,white,col,mine,opp);
        }
    }
}
