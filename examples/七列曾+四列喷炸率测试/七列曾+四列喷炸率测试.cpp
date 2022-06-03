/*
 * @Author: crescendo
 * @Date: 2022-05-29 15:08:18
 * @Last Modified by: crescendo
 * @Last Modified time: 2022-05-29 21:48:01
 * 使用 AvZ Testing 框架测试七列曾+四列喷炸率
 */

#include "avz.h"
#include "avz_testing.h"

using namespace AvZ;
using namespace AZT;

#define TOTAL_TEST_ROUND 51                           // 每一批次测试的选卡数
#define SET_ALL_AS_BIG_WAVE 0                         // 0 - 将所有波次转为普通波；1 - 将所有波次转为旗帜波
#define SKIP_TICK 1                                   // 0 - 慢放，用于检查情况；1 - 开启跳帧测试
#define LOCK_WAVE 1                                   // 0 - 不锁定刷新；1 - 锁定刷新，直至当前波次小丑全部消灭
#define BATCH_SIZE 2                                  // 批次数，对应不同的用冰时机
std::vector<Grid> test_plant_list = {{3, 7}, {4, 7}}; // 数据列表
std::vector<Grid> always_attack_list = {};            // 设为永动攻击的植物列表
std::set<int> jack_rows = {2, 5};                     // 小丑行数
std::vector<int> ice_time_list = {236, 276};          // 要测试的用冰时机

AlwaysAttack aa;
LockWave lw;
std::vector<JackData> jd_list(BATCH_SIZE, JackData());
int completed_round = 0;
int current_jackdata = 0;

void Script()
{
    // 跳帧测试
    auto condition = [=]()
    {
        return current_jackdata < BATCH_SIZE && ice_time_list.size() == BATCH_SIZE;
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
        if (ice_time_list.size() != BATCH_SIZE)
        {
            ShowErrorNotInQueue("测试程序未能执行。原因：BATCH_SIZE设置有误。");
        }
        else
        {
            for (int i = 0; i < BATCH_SIZE; i++)
            {
                ShowErrorNotInQueue("测试完毕，总共测试了#只小丑。\nI=#", (50 * 20 - 10) * TOTAL_TEST_ROUND - 45, ice_time_list[i]);
                jd_list[i].print_stats();
            }
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
    jd_list[current_jackdata].start(test_plant_list); // 开始记录指定植物的炸率数据
    aa.start(always_attack_list);                     // 使指定植物永动
    if (LOCK_WAVE == 1)
    {
        lw.start({JACK_IN_THE_BOX_ZOMBIE});
    }

    // 自由种植功能里已开启蘑菇免唤醒
    for (int w = 1; w <= 20; w++)
    {
        SetTime(ice_time_list[current_jackdata] - 100, w);
        Card(ICE_SHROOM, 1, 1);
    }

    // w20扫尾
    killAllZombie({9}, {}, 1000);

    // 更新已经完成的选卡数
    InsertTimeOperation(0, 20, [=]()
                        { completed_round++;
                        if (completed_round >= TOTAL_TEST_ROUND) {
                            completed_round = 0;
                            current_jackdata++;
                        } });
}