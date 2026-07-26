#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
	/// <summary>
	/// ゲーム中に「Option」アクション(Escapeキー / パッドのMenuボタン)で開く設定メニュー。
	///
	/// 開いている間はTimeScaleを0にしてゲームを停止する(パーク選択と同じ方式)。
	/// 設定値はdata::GameSettingsDataへ書き込み、メニューを閉じたタイミングでJSONへ保存する
	/// (スライダー操作のたびに保存するとファイルI/Oが頻発するため)。
	///
	/// 項目は将来の拡張(画質設定など)を見越して配列で持ち、
	/// 描画・入力はインデックスで一様に扱う。項目を増やす場合はkItemCountと
	/// BuildItemLabelの対応を追加するだけでよい。
	/// </summary>
	class OptionsMenuSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		/// <summary>
		/// メニューの階層。開いた直後はRootを表示し、そこから各画面へ入る。
		/// いきなり音量が出ると「タイトルへ戻る」等の導線が無くなるため階層を分ける。
		/// </summary>
		enum class ePage
		{
			Root,     // 設定 / 操作方法 / タイトルへ / 戻る
			Settings, // 音量など(将来の画質設定もここへ追加する)
			Controls, // 操作方法(読み取り専用の一覧 + 戻る)
		};

		/// <summary>Rootページの項目</summary>
		enum class eRootItem : int
		{
			Settings = 0,
			Controls,
			ReturnToTitle,
			Resume,
			Count
		};

		/// <summary>Settingsページの項目。追加する場合はRefreshLabelsにも対応を足すこと</summary>
		enum class eSettingsItem : int
		{
			MasterVolume = 0,
			BgmVolume,
			SeVolume,
			Back,
			Count
		};

		/// <summary>現在のページの項目数</summary>
		int GetItemCount() const;

		void Open(entt::registry& registry);
		void Close(entt::registry& registry);

		/// <summary>ページを切り替えてUIを作り直す</summary>
		void ChangePage(entt::registry& registry, ePage page);

		/// <summary>カーソル移動・値の増減・決定を処理する</summary>
		void HandleInput(entt::registry& registry);

		/// <summary>現在のページの項目に対する決定操作</summary>
		void Decide(entt::registry& registry);

		/// <summary>表示中のUIエンティティを破棄する</summary>
		void DestroyUi(entt::registry& registry);

		/// <summary>現在のページの項目に合わせてUIエンティティを作る</summary>
		void BuildUi();

		/// <summary>Controlsページ専用: 選択不可の操作方法一覧テキストを作る(mUiEntitiesの末尾に追加、
		/// RefreshLabelsが触る選択項目の範囲外なので固定表示のままになる)</summary>
		void BuildControlsInfoLines();

		/// <summary>ゲーム画面に文字が直接乗ると読みづらいため、生成済みテキストの実測範囲から
		/// 黒半透明の板を動的にサイズして敷く(RefreshLabelsで文字列確定後に呼ぶこと)</summary>
		void BuildBackgroundPanel();

		/// <summary>現在の設定値から各項目の表示文字列を作り直す</summary>
		void RefreshLabels();

		/// <summary>選択中の項目の値をdelta分だけ増減する(音量は0.0〜1.0でクランプ)</summary>
		void AdjustSelected(float delta);

		bool  mIsOpen = false;
		ePage mPage = ePage::Root;
		int   mSelectedIndex = 0;

		// 表示中のUIエンティティ(タイトル行 + 各項目行)。閉じるときにまとめて破棄する
		std::vector<entt::entity> mUiEntities;

		// メニューを開く直前のTimeScale。閉じたときに元へ戻す
		// (ゲーム中以外から開かれた場合に1.0固定で戻すと挙動が変わってしまうため)
		float mPrevTimeScale = 1.0f;
	};
}
