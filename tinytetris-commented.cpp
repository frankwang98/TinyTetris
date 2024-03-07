#include <ctime>
#include <curses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// block layout is: {w-1,h-1}{x0,y0}{x1,y1}{x2,y2}{x3,y3} (two bits each)
// 坐标变换、速度、得分
int x = 431424, y = 598356, r = 427089, px = 247872, py = 799248, pr,
    c = 348480, p = 615696, tick, board[20][10],
    block[7][4] = {{x, y, x, y},
                   {r, p, r, p},
                   {c, c, c, c},
                   {599636, 431376, 598336, 432192},
                   {411985, 610832, 415808, 595540},
                   {px, py, px, py},
                   {614928, 399424, 615744, 428369}},
    score = 0;

// extract a 2-bit number from a block entry
/**
 * @brief 决定方块类型
 * @param x 
 * @param y 
 * @return int 
 */
int NUM(int x, int y) { return 3 & block[p][x] >> y; }

// create a new piece, don't remove old one (it has landed and should stick)
/**
 * @brief 产生新方块（前一个落地后）
 */
void new_piece() {
  // 设置新块的y值，7种块类型，4种旋转类型，x值随机
  y = py = 0;
  p = rand() % 7;
  r = pr = rand() % 4;
  x = px = rand() % (10 - NUM(r, 16));
}

// draw the board and score
/**
 * @brief 画出对应颜色的块和得分
 */
void frame() {
  for (int i = 0; i < 20; i++) {
    move(1 + i, 1); // otherwise the box won't draw
    for (int j = 0; j < 10; j++) {
      board[i][j] && attron(262176 | board[i][j] << 8);
      printw("  ");
      attroff(262176 | board[i][j] << 8);
    }
  }
  move(21, 1);
  printw("Score: %d", score);
  refresh();
}

// set the value fo the board for a particular (x,y,r) piece
/**
 * @brief 给地图（数组）赋值，表示某个方块的位置信息
 * @param x 要绘制的x坐标
 * @param y y坐标
 * @param r 旋转类型
 * @param v 非0:颜色信息 0:擦除方块
 */
void set_piece(int x, int y, int r, int v) {
  for (int i = 0; i < 8; i += 2) {
    board[NUM(r, i * 2) + y][NUM(r, (i * 2) + 2) + x] = v;
  }
}

// move a piece from old (p*) coords to new
/**
 * @brief 更新方块（如消除）
 * @return int 
 */
int update_piece() {
  // 擦除原位置并放入新的位置
  set_piece(px, py, pr, 0);
  set_piece(px = x, py = y, pr = r, p + 1);
  return 0;
}

// remove line(s) from the board if they're full
/**
 * @brief 如果满了就消除行，并增加得分
 */
void remove_line() {
  for (int row = y; row <= y + NUM(r, 18); row++) {
    c = 1;
    // 遍历一行，没满就继续
    for (int i = 0; i < 10; i++) {
      c *= board[row][i];
    }
    if (!c) {
      continue;
    }
    // 满了一行，就依次向下移
    for (int i = row - 1; i > 0; i--) {
      memcpy(&board[i + 1][0], &board[i][0], 40);
    }
    // 清空顶行
    memset(&board[0][0], 0, 10);
    score++;
  }
}

// check if placing p at (x,y,r) will be a collision
/**
 * @brief 判断方块在某位置是否会碰撞
 * @param x 
 * @param y 
 * @param r 
 * @return int 
 */
int check_hit(int x, int y, int r) {
  if (y + NUM(r, 18) > 19) {
    return 1;
  }
  set_piece(px, py, pr, 0);
  c = 0;
  for (int i = 0; i < 8; i += 2) {
    board[y + NUM(r, i * 2)][x + NUM(r, (i * 2) + 2)] && c++;
  }
  set_piece(px, py, pr, p + 1);
  return c;
}

// slowly tick the piece y position down so the piece falls
/**
 * @brief 控制方块下落，嘀嗒
 * @return int 
 */
int do_tick() {
  if (++tick > 30) {
    tick = 0;
    if (check_hit(x, y + 1, r)) {
      if (!y) {
        return 0;
      }
      remove_line();
      new_piece();
    } else {
      y++;
      update_piece();
    }
  }
  return 1;
}

// main game loop with wasd input checking
/**
 * @brief 游戏主循环，通过键盘WASD输入控制方块变换（W旋转）
 */
void runloop() {
  while (do_tick()) {
    usleep(10 * 1000);
    if ((c = getch()) == 'a' && x > 0 && !check_hit(x - 1, y, r)) {
      x--;
    }
    if (c == 'd' && x + NUM(r, 16) < 9 && !check_hit(x + 1, y, r)) {
      x++;
    }
    if (c == 's') {
      // 向下没有冲突就一直向下
      while (!check_hit(x, y + 1, r)) {
        y++;
        update_piece();
      }
      remove_line();
      new_piece();
    }
    if (c == 'w') {
      // 旋转变形
      ++r %= 4;
      while (x + NUM(r, 16) > 9) {
        x--;
      }
      // 如果W变形时有碰撞，则不能变形
      if (check_hit(x, y, r)) {
        x = px;
        r = pr;
      }
    }
    if (c == 'q') {
      return;
    }
    update_piece();
    frame();
  }
}

// init curses and start runloop
/**
 * @brief 初始化curses并开始循环
 * @return int 
 */
int main() {
  // 初始化随机种子，进入和退出curses，初始化颜色
  srand(time(0));
  initscr();
  start_color();
  // colours indexed by their position in the block
  // 方块的颜色映射它们的位置信息
  for (int i = 1; i < 8; i++) {
    init_pair(i, i, 0);
  }
  new_piece();
  // 设置窗口大小，不回显，不显示光标，绘制边界
  resizeterm(22, 22);
  noecho();
  timeout(0);
  curs_set(0);
  box(stdscr, 0, 0);
  runloop();
  endwin();
}
