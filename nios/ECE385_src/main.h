#ifndef MAIN_H_
#define MAIN_H_

#include "comm.h"

extern volatile int* io_led_red;
extern volatile int* io_led_green;
extern volatile int* io_hex;
extern volatile int* io_vga_sync;
extern volatile int* io_vga_background_offset;

extern int time_start;
extern int player_scores[5];

typedef enum {
	PREPARE_GAME = 0,	// 准备游戏
	IN_GAME = 1,	// 单人游戏正在进行
	GAME_OVER = 2,	// 单人游戏结束
	GAME_OVER_UPLOAD_SCORE_BEGIN = 3,	// 开始上传分数
	GAME_OVER_UPLOAD_SCORE_PROCESSING = 4,	// 正在上传分数
	GAME_OVER_UPLOAD_SCORE_FINISH = 5,	// 上传分数完成
	GAME_OVER_WAIT_ENTER_PRESS = 6,	// 游戏结束，按键返回
	GAME_OVER_WAIT_ENTER_PRESS_OR_R_PRESS = 7,	// 按键R刷新积分表
	GAME_OVER_WAIT_ENTER_RELEASE_TO_SCOREBOARD = 8,	// 等待更新积分榜
	GAME_OVER_WAIT_R_RELEASE = 9,	// 等待R键
	MAIN_MENU_PREPARE = 10,	// 预加载主界面
	MAIN_MENU = 11,	// 进入主界面
	SCOREBOARD_PREPARE = 12,// 预加载积分榜
	SCOREBOARD = 13,	// 积分榜界面
	GAME_OVER_WAIT_ENTER_RELEASE_TO_MENU = 14,	// 游戏结束等待返回界面
	PREPARE_GAME_DOUBLE = 15,	// 准备双人模式
	IN_GAME_double = 16,	// 双人模式正在进行
	GAME_OVER_double = 17,	// 双人模式结束
} game_state_t;

extern volatile game_state_t game_state;

int enter_pressed();
int r_pressed();
int esc_pressed();
int p_pressed();
int main(void);

#endif /* MAIN_H_ */
