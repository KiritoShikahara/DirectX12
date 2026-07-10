#pragma once

#include<Data/Enemy/EnemyData.h>
#include<string>

namespace debug
{
	class EnemyStatusDebugPanel
	{
    public:
        static void RegisterDebugUI(const std::string& DebugKey);
        static void UnregisterDebugUI();
    private:

        static void Draw();
        static void ApplyRowToEnemies(const data::EnemyData& row);

        static std::string mDebugKey;
    };
}


