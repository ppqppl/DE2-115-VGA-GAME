#ifndef SPRITES_H_
#define SPRITES_H_

#include <stdint.h>
#include <unistd.h>

typedef struct {
	int32_t x;
	int32_t y;
	union {
		// For airplane
		struct {
			int32_t width;
			int32_t height;
		};
		// For bullet
		struct {
			int32_t bullet_radius;
			int32_t bullet_color;
		};
	};
} vga_entity_t;

// Data structure for software handling of sprite.
typedef struct {	// 玩家
	// Pointer to physical entity, exposed as vga_sprite_params in Platform Designer.
	// NULL means this sprite is not visible on the screen

	//	指向玩家实体的结构体，被引用为vga_sprite_params。
	// 	NULL表示这个精灵在屏幕上不可见
	volatile vga_entity_t* physical;	// 玩家实体
	volatile uint16_t* sprite_data;		// 玩家数据
	int32_t used;			// 是否被使用
	int32_t vx;				// x轴移动速度
	int32_t vx_max;			// x轴最大移动速度
	int32_t vx_min;			// x轴最小游动速度
	int32_t vy;				// y轴移动速度
	int32_t vy_max;			// y轴最大移动速度
	int32_t vy_min;			// y轴最小移动速度
	int32_t ax;				// x轴移动加速度
	int32_t ay;				// y轴移动加速度
	int32_t type;			// 实体标志符号
	// 0: entity of player	        玩家实体
	// 1: entity of enemy	        敌人实体
	// 2: explo~sion		        爆炸
	int32_t hp;				// 血量
	// for planes of player & enemy, times of collisions until death
	// for explo~sion, frame ticks until death
	int32_t frame_created;	// frame at which sprite is created  创建的玩家的帧
	uint16_t bullet_color;	// 子弹颜色
} vga_sprite_info_t;

// Data structure for operations against different types of sprites.
// 图片存储数据
typedef struct {
	uint8_t max_size;
	uint16_t** mmap_sprite_data;
	volatile vga_entity_t* mmap_addr;
	volatile vga_sprite_info_t* info_arr;
} vga_entity_manage_t;

// 敌人飞机和子弹数量
#define N_PLANES 8
#define N_BULLETS 56

// 敌人飞机和子弹
extern vga_entity_manage_t vga_planes;
extern vga_entity_manage_t vga_bullets;

// 1P 玩家和子弹
#define VGA_SPRITE_PLANE &vga_planes
#define VGA_SPRITE_BULLET &vga_bullets

// 2P 玩家和子弹
#define VGA_SPRITE_PLANE_2p &vga_planes
#define VGA_SPRITE_BULLET_2p &vga_bullets

// 玩家飞机宽度和位移宽度
#define VGA_SPRITE_HW_SHIFT_BITS 4
#define VGA_SPRITE_WIDTH (VGA_WIDTH << VGA_SPRITE_HW_SHIFT_BITS)
#define VGA_SPRITE_HEIGHT (VGA_HEIGHT << VGA_SPRITE_HW_SHIFT_BITS)

// 玩家和敌人飞机爆炸时的减速占比
#define PLAYER_PLANE_EXPLOSION_SLOWDOWN_RATIO 8
#define ENEMY_PLANE_EXPLOSION_SLOWDOWN_RATIO 2


	// 初始化飞机实体
	void sprites_init(vga_entity_manage_t* vga_entity_type);
	// 分配飞机实体，并返回ID
	uint8_t sprites_allocate(vga_entity_manage_t* vga_entity_type);
	// 获取飞机ID的指针变量
	volatile vga_sprite_info_t* sprites_get(vga_entity_manage_t* vga_entity_type, uint8_t id);
	// 清空ID变量
	uint8_t sprites_deallocate(vga_entity_manage_t* vga_entity_type, uint8_t id);
	// 加载飞机数据
	uint8_t sprites_load_data(vga_entity_manage_t* vga_entity_type, uint8_t id, const uint16_t* src, int32_t pixel_count);

	//	1p
	// 限制速度设置
	uint8_t sprites_limit_speed(vga_entity_manage_t* vga_entity_type, uint8_t id);		//  1p
	// 更新飞机状态
	uint8_t sprites_tick(vga_entity_manage_t* vga_entity_type);		// 1p
	// 检测飞机于敌人碰撞
	int32_t sprites_collision_detect();		// 1p
	// 判断飞机是否存在，同时可以作为死亡判断
	uint8_t sprites_visible(vga_entity_manage_t* vga_entity_type, uint8_t id);


//	2p
uint8_t sprites_limit_speed_2p(vga_entity_manage_t* vga_entity_type, uint8_t id);		//  2p
uint8_t sprites_tick_2p(vga_entity_manage_t* vga_entity_type);		// 2p
int32_t sprites_collision_detect_2p();		// 2p
uint8_t sprites_visible_2p(vga_entity_manage_t* vga_entity_type, uint8_t id);

#endif
