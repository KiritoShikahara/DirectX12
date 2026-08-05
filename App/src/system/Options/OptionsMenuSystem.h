#pragma once

#include<ecs/system/manager/IComponentSystem.h>
#include<entt/entt.hpp>
#include<vector>

namespace ecs
{
	///<summary>
	///ゲーム中にOptionアクションで開く設定メニュー。開いている間はTimeScaleを0にし、設定値はメニューを閉じたタイミングでJSONへ保存する。項目は配列で持ちインデックスで一様に扱う
	///</summary>
	class OptionsMenuSystem : public IUserSystem
	{
	public:
		void Update(entt::registry& registry, float deltaTime, float rawDeltaTime) override;

	private:
		///<summary>
		///メニューの階層。開いた直後はRootを表示し、そこから各画面へ入る
		///</summary>
		enum class ePage
		{
			Root,     // 設定 / 操作方法 / タイトルへ / 戻る
			Settings, // 音量など、将来の画質設定もここへ追加する
			Controls, // 操作方法、読み取り専用の一覧と戻る
		};

		///<summary>
		///Rootページの項目
		///</summary>
		enum class eRootItem : int
		{
			Settings = 0,
			Controls,
			ReturnToTitle,
			Resume,
			Count
		};

		///<summary>
		///Settingsページの項目。追加する場合はRefreshLabelsにも対応を足すこと
		///</summary>
		enum class eSettingsItem : int
		{
			MasterVolume = 0,
			BgmVolume,
			SeVolume,
			Back,
			Count
		};

		///<summary>
		///現在のページの項目数
		///</summary>
		int GetItemCount() const;

		void Open(entt::registry& registry);
		void Close(entt::registry& registry);

		///<summary>
		///現在のGameStateがメニューを開ける状態か判定する。InGame/PerkSelectのみ
		///</summary>
		bool CanOpen(entt::registry& registry) const;

		///<summary>
		///GameStateComponent.IsOptionsMenuOpenを書き換える。存在しなければ何もしない
		///</summary>
		void SetOptionsMenuOpenFlag(entt::registry& registry, bool isOpen);

		///<summary>
		///ページを切り替えてUIを作り直す
		///</summary>
		void ChangePage(entt::registry& registry, ePage page);

		///<summary>
		///カーソル移動・値の増減・決定を処理する
		///</summary>
		void HandleInput(entt::registry& registry);

		///<summary>
		///現在のページの項目に対する決定操作
		///</summary>
		void Decide(entt::registry& registry);

		///<summary>
		///表示中のUIエンティティを破棄する
		///</summary>
		void DestroyUi(entt::registry& registry);

		///<summary>
		///現在のページの項目に合わせてUIエンティティを作る
		///</summary>
		void BuildUi();

		///<summary>
		///Controlsページ専用、選択不可の操作方法一覧テキストを作る。mUiEntitiesの末尾に追加しRefreshLabelsの対象範囲外なので固定表示のままになる
		///</summary>
		void BuildControlsInfoLines();

		///<summary>
		///背景パネルを敷く。Controlsページは内容に合わせたサイズ(BuildFittedControlsPanel)、
		///それ以外のページは画面全体を覆うサイズにする
		///</summary>
		void BuildBackgroundPanel();

		///<summary>
		///Controlsページ専用、生成済みテキストの実測範囲から一回り大きい余白を持つ黒不透明パネルを敷く。
		///対象のテキストが1件も無ければ何もせずfalseを返す
		///</summary>
		bool BuildFittedControlsPanel();

		///<summary>
		///文字は常にスプライトより後段で描画される仕様のため、パネル(スプライト)だけでは他の文字を隠せない。
		///Open()時点で表示中だった、このメニュー以外のTextComponentをmSuppressedTextへ記録して非表示にする
		///</summary>
		void SuppressOtherText(entt::registry& registry);

		///<summary>
		///mSuppressedTextに登録済みの文字を毎フレーム再度非表示にする。
		///WeaponIconBarSystem等、毎フレームIsVisibleを設定し直すシステムに上書きされてしまうため、
		///OptionsMenuSystemが他システムより後段に登録されていることを前提に、開いている間は毎フレーム呼ぶこと
		///</summary>
		void HideSuppressedText(entt::registry& registry);

		///<summary>
		///SuppressOtherTextで非表示にした文字を元の表示状態(IsVisible=true)へ戻す
		///</summary>
		void RestoreSuppressedText(entt::registry& registry);

		///<summary>
		///現在の設定値から各項目の表示文字列を作り直す
		///</summary>
		void RefreshLabels();

		///<summary>
		///選択中の項目の値をdelta分だけ増減する。音量は0.0から1.0でクランプ
		///</summary>
		void AdjustSelected(float delta);

		bool  mIsOpen = false;
		ePage mPage = ePage::Root;
		int   mSelectedIndex = 0;

		///<summary>
		///表示中のUIエンティティ、タイトル行と各項目行。閉じるときにまとめて破棄する
		///</summary>
		std::vector<entt::entity> mUiEntities;

		///<summary>
		///メニューを開く直前のTimeScale。閉じたときに元へ戻す
		///</summary>
		float mPrevTimeScale = 1.0f;

		///<summary>
		///SuppressOtherTextで非表示にした、このメニュー以外のテキストエンティティ。Close()で元に戻す
		///</summary>
		std::vector<entt::entity> mSuppressedText;
	};
}
