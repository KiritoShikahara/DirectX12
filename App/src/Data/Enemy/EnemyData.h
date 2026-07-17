#pragma once

#include<string>
#include<Data/Storage/Reflection.h>

namespace data
{
    /// <summary>
    /// �G�̏����X�e�[�^�X �}�X�^�iCSV / DB�j�B
    /// EnemyStatusComponent �� Base�iMaxHp / MoveSpeed / AtkPower�j�֑Ή�����B
    /// CSV �w�b�_���͊e�t�B�[���h���Ɗ��S��v�����邱�ƁB
    /// </summary>
    struct EnemyData
    {
        int         Id = 0;     // �G��ID�i��L�[�BEnemyStatusComponent::EnemyId �ƑΉ��j
        std::string Name;              // �\�����E�f�o�b�O�p
        float       MaxHp = 10.0f; // �ő�HP
        float       MoveSpeed = 2.0f;  // �ړ����x m/s
        float       AtkPower = 1.0f;  // �ڐG�_���[�W
        int         Exp = 3;     // ���j���̊l���o���l
        float       GoldValue = 3.0f; // Gold awarded to the player on kill (EnemyDeathSystem::AwardGold)

        REFLECT_BEGIN(EnemyData, "enemies")
            REFLECT_FIELD_ID(Id)
            REFLECT_FIELD_STR(Name)
            REFLECT_FIELD_FLOAT(MaxHp)
            REFLECT_FIELD_FLOAT(MoveSpeed)
            REFLECT_FIELD_FLOAT(AtkPower)
            REFLECT_FIELD_INT(Exp)
            REFLECT_FIELD_FLOAT(GoldValue)
            REFLECT_END()
    };
}

REFLECT_REGISTER(data::EnemyData);