#pragma once

namespace ecs
{
    // �V�[�����ׂ��Ő�������
    struct PersistentTag {};

    // ���݂̃t���[���̍Ō�ɍ폜�����\��
    struct PendingDestroyTag {};

    // �ҏW���̃G�f�B�^��ł̂ݑI������Ă���
    struct SelectedTag {};

    // �J�����ɉf��Ώۂł���
    struct RenderableTag {};

    // エディタで配置されたエンティティ。Save/Load/スナップショットの対象になる。
    struct PlaceableTag {};
}