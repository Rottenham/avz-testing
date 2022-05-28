/*
 * @Author: crescendo
 * @Date: 2022-05-28 08:45:31
 * @Last Modified by: crescendo
 * @Last Modified time: 2022-05-28 10:29:46
 *  __AVZ_VERSION__ == 220213
 */

#pragma once
#include "libavz.h"

#ifndef __AZT_VERSION__
#define __AZT_VERSION__ 1.0
#endif

namespace AZT
{
    struct ZombieRowInfo
    {
        ZombieType zombie_type;
        std::set<int> rows;
    };

    struct ZombieAZT : public Zombie
    {
        int &layer()
        {
            return (int &)((uint8_t *)this)[0x20];
        }
    };

    std::pair<int, int> getDefenseRange(PlantType type)
    {
        switch (type)
        {
        case TALL_NUT:
            return {-50, 30};
        case PUMPKIN:
            return {-60, 40};
        case COB_CANNON:
            return {-60, 80};
        default:
            return {-50, 10};
        }
    }

    bool judgeExplode(SafePtr<Zombie> &z, SafePtr<Plant> &p)
    {
        int x = z->abscissa(), y = z->ordinate();
        int y_dis = 0;
        if (y < p->yi() - 60)
            y_dis = (p->yi() - 60) - y;
        else if (y > p->yi() + 20)
            y_dis = y - (p->yi() + 20);
        if (y_dis > 90)
            return false;
        int x_dis = sqrt(90 * 90 - y_dis * y_dis);
        auto def = getDefenseRange(PlantType(p->type()));
        return p->xi() + def.first - x_dis <= x && x <= p->xi() + def.second + x_dis;
    }

    void killZombie(SafePtr<Zombie> zombie)
    {
        zombie->state() = 3;
    }

    class ZombieEye : public AvZ::TickRunner
    {
    private:
        int callback_num = 0;
        void run(std::function<bool(SafePtr<Zombie>)> condition, std::function<void(SafePtr<Zombie>)> callback)
        {
            SafePtr<Zombie> zombie = (Zombie *)AvZ::GetMainObject()->zombieArray();
            int zombies_count_max = AvZ::GetMainObject()->zombieTotal();
            for (int i = 0; i < zombies_count_max; ++i, ++zombie)
            {
                if (zombie->isDead() || zombie->isDisappeared() || !zombie->isExist())
                {
                    continue;
                }
                if (condition(zombie))
                {
                    callback(zombie);
                }
            }
        }

    public:
        // ***In Queue
        void start(std::function<bool(SafePtr<Zombie>)> condition, std::function<void(SafePtr<Zombie>)> callback)
        {
            AvZ::InsertOperation([=]()
                                 { pushFunc([=]()
                                            { run(condition, callback); }); });
        }
        int getCallbackNum()
        {
            return callback_num;
        }
        int incCallbackNum(int diff = 1)
        {
            callback_num += diff;
        }
    };

    class JackEye : public ZombieEye
    {
    private:
        std::vector<std::pair<AvZ::Grid, int>> jack_stats;
        void incJackNum(AvZ::Grid plant_position)
        {
            for (auto it = jack_stats.begin(); it != jack_stats.end(); it++)
            {
                if ((*it).first == plant_position)
                {
                    (*it).second++;
                    return;
                }
            }
        }
        void run(std::vector<AvZ::Grid> plant_list)
        {
            SafePtr<Plant> plant = (Plant *)AvZ::GetMainObject()->plantArray();
            std::vector<SafePtr<Plant>> final_plant_list;
            for (auto g : plant_list)
            {
                int idx = AvZ::GetPlantIndex(g.row, g.col);
                if (idx >= 0)
                    final_plant_list.push_back(plant + idx);
            }
            SafePtr<Zombie> zombie = (Zombie *)AvZ::GetMainObject()->zombieArray();
            int zombies_count_max = AvZ::GetMainObject()->zombieTotal();
            for (int i = 0; i < zombies_count_max; ++i, ++zombie)
            {
                if (zombie->isDead() || zombie->isDisappeared() || !zombie->isExist())
                {
                    continue;
                }
                if ((zombie->type() == JACK_IN_THE_BOX_ZOMBIE && zombie->state() == 16))
                {
                    for (auto p : final_plant_list)
                    {
                        if (judgeExplode(zombie, p))
                        {
                            incJackNum({p->row() + 1, p->col() + 1});
                        }
                    }
                    killZombie(zombie);
                }
            }
        }

    public:
        // ***In Queue
        void start(const std::vector<AvZ::Grid> plant_list)
        {
            for (auto p : plant_list)
            {
                jack_stats.push_back({p, 0});
            }
            AvZ::InsertOperation([=]()
                                 { pushFunc([=]()
                                            { run(plant_list); }); });
        }

        void print_stats()
        {
            std::stringstream ss;
            ss << "[Jack Eye Stats]\n";
            bool all_zero = true;
            for (auto s : jack_stats)
            {
                if (s.second == 0)
                    continue;
                ss << s.first.row << "-" << s.first.col << ": " << s.second << std::endl;
                if (all_zero)
                {
                    all_zero = false;
                }
            }
            if (all_zero)
            {
                ss << "NO EXPLOSION\n";
            }
            AvZ::ShowErrorNotInQueue("#", ss.str());
        }
    };

    // 暂停出怪
    void zombieSpawnPause(bool f = true)
    {
        if (f)
        {
            AvZ::WriteMemory<uint8_t>(0xeb, 0x004265dc);
        }
        else
        {
            AvZ::WriteMemory<uint8_t>(0x74, 0x004265dc);
        }
    }

    // 植物无敌
    void plantInvincible(bool f = true)
    {
        if (f)
        {
            AvZ::WriteMemory(std::array<uint8_t, 3>{0x46, 0x40, 0x00}, 0x0052fcf1);
            AvZ::WriteMemory<uint8_t>(0xeb, 0x0041cc2f);
            AvZ::WriteMemory<uint8_t>(0xeb, 0x005276ea);
            AvZ::WriteMemory(std::array<uint8_t, 3>{0x90, 0x90, 0x90}, 0x0046cfeb);
            AvZ::WriteMemory(std::array<uint8_t, 3>{0x90, 0x90, 0x90}, 0x0046d7a6);
            AvZ::WriteMemory<uint8_t>(0xeb, 0x0052e93b);
            AvZ::WriteMemory<uint8_t>(0x70, 0x0045ee0a);
            AvZ::WriteMemory<uint8_t>(0x00, 0x0045ec66);
            AvZ::WriteMemory(std::array<uint8_t, 3>{0xc2, 0x04, 0x00}, 0x00462b80);
        }
        else
        {
            AvZ::WriteMemory(std::array<uint8_t, 3>{0x46, 0x40, 0xfc}, 0x0052fcf1);
            AvZ::WriteMemory<uint8_t>(0x74, 0x0041cc2f);
            AvZ::WriteMemory<uint8_t>(0x75, 0x005276ea);
            AvZ::WriteMemory(std::array<uint8_t, 3>{0x29, 0x50, 0x40}, 0x0046cfeb);
            AvZ::WriteMemory(std::array<uint8_t, 3>{0x29, 0x4e, 0x40}, 0x0046d7a6);
            AvZ::WriteMemory<uint8_t>(0x74, 0x0052e93b);
            AvZ::WriteMemory<uint8_t>(0x75, 0x0045ee0a);
            AvZ::WriteMemory<uint8_t>(0xce, 0x0045ec66);
            AvZ::WriteMemory(std::array<uint8_t, 3>{0x53, 0x55, 0x8b}, 0x00462b80);
        }
    }

    // 僵尸进家后不食脑
    void forbidEnterHome(bool f = true)
    {
        if (f)
        {
            AvZ::WriteMemory(std::array<uint8_t, 2>{0x90, 0x90}, 0x52b308);
        }
        else
        {
            AvZ::WriteMemory(std::array<uint8_t, 2>{0x74, 0x07}, 0x52b308);
        }
    }

    // 僵尸不掉钱
    void forbidItemDrop(bool f = true)
    {
        if (f)
        {
            AvZ::WriteMemory(std::array<uint8_t, 2>{0x90, 0xe9}, 0x0041d025);
        }
        else
        {
            AvZ::WriteMemory(std::array<uint8_t, 2>{0x0f, 0x8f}, 0x0041d025);
        }
    }

    // 自由种植（无视阳光+取消冷却+紫卡直种+随意放置+蘑菇免唤醒）
    void freePlanting(bool f = true)
    {
        if (f)
        {
            // 无视阳光
            AvZ::WriteMemory<uint8_t>(0x70, 0x0041ba72);
            AvZ::WriteMemory<uint8_t>(0x3b, 0x0041ba74);
            AvZ::WriteMemory<uint8_t>(0x91, 0x0041bac0);
            AvZ::WriteMemory<uint8_t>(0x80, 0x00427a92);
            AvZ::WriteMemory<uint8_t>(0x80, 0x00427dfd);
            AvZ::WriteMemory<uint8_t>(0xeb, 0x0042487f);

            // 取消冷却
            AvZ::WriteMemory<uint8_t>(0x70, 0x00487296);
            AvZ::WriteMemory<uint8_t>(0xeb, 0x00488250);

            // 紫卡直种
            AvZ::WriteMemory(std::array<uint8_t, 3>{0xb0, 0x01, 0xc3}, 0x0041d7d0);
            AvZ::WriteMemory<uint8_t>(0xeb, 0x0040e477);

            // 随意放置
            AvZ::WriteMemory<uint8_t>(0x81, 0x0040fe30);
            AvZ::WriteMemory<uint8_t>(0xeb, 0x00438e40);
            AvZ::WriteMemory<uint8_t>(0x8d, 0x0042a2d9);

            // 蘑菇免唤醒
            AvZ::WriteMemory<uint8_t>(0xeb, 0x45de8e);
        }
        else
        {
            // 取消无视阳光
            AvZ::WriteMemory<uint8_t>(0x7f, 0x0041ba72);
            AvZ::WriteMemory<uint8_t>(0x2b, 0x0041ba74);
            AvZ::WriteMemory<uint8_t>(0x9e, 0x0041bac0);
            AvZ::WriteMemory<uint8_t>(0x8f, 0x00427a92);
            AvZ::WriteMemory<uint8_t>(0x8f, 0x00427dfd);
            AvZ::WriteMemory<uint8_t>(0x74, 0x0042487f);

            // 恢复冷却
            AvZ::WriteMemory<uint8_t>(0x7e, 0x00487296);
            AvZ::WriteMemory<uint8_t>(0x75, 0x00488250);

            // 取消紫卡直种
            AvZ::WriteMemory(std::array<uint8_t, 3>{0x51, 0x83, 0xf8}, 0x0041d7d0);
            AvZ::WriteMemory<uint8_t>(0x74, 0x0040e477);

            // 取消随意放置
            AvZ::WriteMemory<uint8_t>(0x84, 0x0040fe30);
            AvZ::WriteMemory<uint8_t>(0x74, 0x00438e40);
            AvZ::WriteMemory<uint8_t>(0x84, 0x0042a2d9);

            // 取消蘑菇免唤醒
            AvZ::WriteMemory<uint8_t>(0x74, 0x45de8e);
        }
    }

    // ***Not In Queue
    // 移动单波僵尸
    void moveZombieOne(ZombieType zombie_type, const std::set<int> &rows, int height, int baseY)
    {
        SafePtr<ZombieAZT> zombie = (ZombieAZT *)AvZ::GetMainObject()->zombieArray();
        int zombies_count_max = AvZ::GetMainObject()->zombieTotal();
        auto it = rows.begin();
        for (int i = 0; i < zombies_count_max; ++i, ++zombie)
        {
            if (zombie->isDead() || zombie->isDisappeared() || !zombie->isExist())
            {
                continue;
            }

            // 只移动本波僵尸
            if (zombie->existTime() > 100)
            {
                continue;
            }

            // 检测类型
            if (zombie->type() != zombie_type)
            {
                continue;
            }

            int row = zombie->row();
            int new_row = *it;
            int diff = new_row - row;
            if (diff != 0)
            {
                zombie->row() = new_row;
                zombie->ordinate() = baseY + new_row * height;
                zombie->layer() += 10000 * diff;
                it++;
                if (it == rows.end())
                {
                    it = rows.begin();
                }
            }
        }
    }

    // 移动 w1~w20 多波僵尸
    void moveZombieMany(ZombieType zombie_type, const std::set<int> &rows)
    {
        if (zombie_type < 0 || zombie_type > 32)
        {
            return;
        }
        std::set<int> rows_converted; // 0~5
        for (auto r : rows)
        {
            if (r >= 1 && r <= 6)
            {
                rows_converted.emplace(r - 1);
            }
        }
        if (rows_converted.empty())
        {
            return;
        }
        int scene = AvZ::GetMainObject()->scene();
        int height = scene == 0 || scene == 1 ? 100 : 85;
        int baseY = scene == 0 || scene == 1 || scene == 2 || scene == 3 ? 50 : 40;
        for (auto w = 1; w <= 20; w++)
        {
            AvZ::InsertTimeOperation(1, w, [=]()
                                     { moveZombieOne(zombie_type, rows_converted, height, baseY); });
        }
    }

    // 将僵尸移动至特定行
    // ***使用示例：
    // moveZombieRow({{JACK_IN_THE_BOX_ZOMBIE, {2, 5}}, {LADDER_ZOMBIE, {1, 6}}});
    void moveZombieRow(const std::vector<ZombieRowInfo> &zombie_row_info)
    {
        for (const auto &zri : zombie_row_info)
        {
            moveZombieMany(zri.zombie_type, zri.rows);
        }
    }

    // 将僵尸移动至特定行
    // ***使用示例：
    // moveZombieRow(JACK_IN_THE_BOX_ZOMBIE, {2, 5});
    void moveZombieRow(ZombieType zombie_type, std::set<int> rows)
    {
        moveZombieRow({{zombie_type, rows}});
    }

    void killAllZombie()
    {
        SafePtr<Zombie> zombie = (Zombie *)AvZ::GetMainObject()->zombieArray();
        int zombies_count_max = AvZ::GetMainObject()->zombieTotal();
        for (int i = 0; i < zombies_count_max; ++i, ++zombie)
        {
            if (zombie->isDead() || zombie->isDisappeared() || !zombie->isExist())
            {
                continue;
            }
            zombie->state() = 3;
        }
    }

    void killAllZombie(int wave, int time)
    {
        AvZ::InsertTimeOperation(time, wave, [=]()
                                 { killAllZombie(); });
    }

    // 秒杀全部僵尸
    // ***使用示例：
    // killAllZombie({10, 20}); // w10、w20僵尸刷出后立刻秒杀
    // killAllZombie({1}, 401); // w1僵尸刷出401cs后秒杀
    void killAllZombie(const std::set<int> &wave, int time = 1)
    {
        if (time < 0)
        {
            time = 1;
        }
        for (auto w : wave)
        {
            if (w >= 1 && w <= 20)
            {
                killAllZombie(w, time);
            }
        }
    }

} // namespace AZT
