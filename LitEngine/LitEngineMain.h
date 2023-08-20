#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"
#include "Common\Model.h"

//class Model;

// Renders Direct3D content on the screen.
namespace LitEngine
{
	class LitEngineMain
	{
	public:
		LitEngineMain();
		void CreateRenderers(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void Update();
		bool Render();

		void OnWindowSizeChanged();
		void OnSuspending();
		void OnResuming();
		void OnDeviceRemoved();

	private:
		// TODO: Replace with your own content renderers.
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;

		// Rendering loop timer.
		DX::StepTimer m_timer;

		Model* m_Glenna3DModel;
		Model* m_CubeModel;

		Model* m_CurrentRenderedModel;
	};
}