/*
 * @Author: crescendo
 * @Date: 2022-05-29 15:08:18
 * @Last Modified by: crescendo
 * @Last Modified time: 2022-05-29 21:48:01
 * 使用 AvZ Testing 框架测试七里曾+四列喷炸率
 */

#include "avz.h"
#include "avz_testing.h"

// ***配置部分
#define TOTAL_TEST_ROUND 51                                              // 每一批次测试的选卡数
#define SET_ALL_AS_BIG_WAVE 0                                            // 0 - 将所有波次转为普通波；1 - 将所有波次转为旗帜波
#define SKIP_TICK 1                                                      // 0 - 慢放，用于检查情况；1 - 开启跳帧测试
#define BATCH_SIZE 3                                                     // 批次数，对应不同的用冰时机
std::vector<Grid> test_plant_list = {{3, 7}, {4, 7}};                    // 要统计炸率的植物列表
std::vector<Grid> always_attack_list = {{3, 7}, {4, 7}, {2, 4}, {5, 4}}; // 设为永动攻击的植物列表
std::set<int> jack_rows = {2, 5};                                        // 小丑行数
const std::vector<int> ice_time_list = {316, 356, 396};                  // 要测试的用冰时机
// ***配置部分结束

using namespace AvZ;
using namespace AZT;

std::vector<JackData> jd_list(BATCH_SIZE, JackData()); // 创建多个记录对象
AlwaysAttack aa;
int completed_round = 0;  // 当前批次已完成的选卡数
int current_jackdata = 0; // 当前所在批次

void Script()
{
    assert(ice_time_list.size() == BATCH_SIZE); // 防止误输入

    // 跳帧测试
    auto condition = [=]()
    {
        return current_jackdata < BATCH_SIZE;
    };
    auto callback = [=]()
    {
        OpenMultipleEffective('W', -1);
        SetErrorMode(POP_WINDOW);
        SetGameSpeed(0.2);

        // 关闭修改功能
        forbidItemDrop(false);
        forbidEnterHome(false);
        plantInvincible(false);
        freePlanting(false);

        // 显示统计结果
        for (int i = 0; i < BATCH_SIZE; i++)
        {
            showErrorNotInQueue("测试完毕，总共测试了#只小丑。\nI=#", (50 * 20 - 10) * TOTAL_TEST_ROUND - 45, ice_time_list[i]);
            jd_list[i].print_stats();
        }
    };
    if (SKIP_TICK == 1)
        SkipTick(condition, callback);

    OpenMultipleEffective('Q', AvZ::MAIN_UI_OR_FIGHT_UI);
    SetErrorMode(NONE);
    SetGameSpeed(10);

    // 设置出怪与选卡
    SetZombies({JACK_IN_THE_BOX_ZOMBIE});
    SelectCards({ICE_SHROOM, M_ICE_SHROOM, 40, 41, 42, 43, 44, 45, 46, 47});

    // 开启修改功能
    forbidItemDrop();  // 僵尸不掉钱
    forbidEnterHome(); // 僵尸不进家
    plantInvincible(); // 植物无敌
    freePlanting();    // 自由种植

    // 将小丑僵尸平均分配至指定路
    moveZombieRow(JACK_IN_THE_BOX_ZOMBIE, jack_rows);

    // 统一波次
    if (SET_ALL_AS_BIG_WAVE == 1)
        unifyAllWaves(BIG_WAVE);
    else
        unifyAllWaves(NORMAL_WAVE);

    // 秒杀旗帜波小偷
    killAllZombie({10, 20}, {BUNGEE_ZOMBIE});

    SetTime(-599, 1);
    jd_list[current_jackdata].start(test_plant_list); 	// 开始记录指定植物的炸率数据
    aa.start(always_attack_list);                		// 使指定植物永动

    // 自由种植功能里已开启蘑菇免唤醒
    for (int w = 1; w <= 20; w++)
    {
        if (!RangeIn(w, {9, 19, 20}))
        {
            SetWavelength({{w, 2236 - ice_time_list[current_jackdata]}}); // 波长不能锁太长，因为所有僵尸死亡后仍会刷出下一波，AvZ锁定波长失效
        }
        SetTime(ice_time_list[current_jackdata] - 100, w);
        Card(ICE_SHROOM, 1, 1);
    }

    // w20扫尾
    killAllZombie({20}, {}, 3000);

    // 更新已经完成的选卡数
    InsertTimeOperation(0, 20, [=]()
                        { completed_round++;
                        if (completed_round >= TOTAL_TEST_ROUND) {
                            completed_round = 0;
                            current_jackdata++;
                        } });
}