#include "pch.h"
#include "LitEngineMain.h"
#include "Common\DirectXHelper.h"

using namespace LitEngine;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

// The DirectX 12 Application template is documented at https://go.microsoft.com/fwlink/?LinkID=613670&clcid=0x409

//Daniel Tutorial
//How to fix missing dll issue: https://www.reddit.com/r/cpp_questions/comments/aktpu5/cant_run_exe_file_because_of_missing_dll_file/

// Loads and initializes application assets when the application is loaded.
LitEngineMain::LitEngineMain()
{
	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
}

// Creates and initializes the renderers.
/*
	This function is only called once!
*/
void LitEngineMain::CreateRenderers(const std::shared_ptr<DX::DeviceResources>& deviceResources)
{
	// TODO: Replace this with your app's content initialization.
	/*
	*	Our device and its resources has now been created by App.cpp (App::main()-> some external calls which lead to ->App::Run()), and is sent into this function.
	*	
	*	The deviceResources is now sent to sampleScene3DRenderer! which creates program related resources used by the pass m_SceneRenderer (see m_sceneRenderer as a renderphase/renderpass).
	*	Sample3DSceneRenderer creates renderpass specific resources such as rendertargets (i.e gbuffers maybe, ssao textures etc), shaders, constant buffers, etc..
	* 
	*	And then it sets all of those to the shader in Sample3DSceneRenderer::Render() (called by our own LitEngineMain::Render()), using the commandList!
	*	Then calls a commandList->Draw(). All these commands are recorded and executed somewhere later on!
	* 
	*	Interesting! So m_SceneRenderer is actually a renderpass! like e.g SSAO pass.
	* 
	*	So if we wanna make this deferred, we have to create global Gbuffer rendertargets right here that will be usable by any renderpass.
	*	Then create multiple renderpasses that will be able to access these global gbuffer RTs.
	* 
	*/

	//Daniel: TODO: Create the 3D model here!
	const char glennaFilepath[255] = "../Assets/3DModels/Harmony/Harmony.txt";
	m_Glenna3DModel = new Model(true);
	m_CubeModel = new Model();

	m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(deviceResources, m_Glenna3DModel));

	OnWindowSizeChanged();
}

// Updates the application state once per frame.
void LitEngineMain::Update()
{
	// Update scene objects.
	m_timer.Tick([&]()
	{
		// TODO: Replace this with your app's content update functions.
		m_sceneRenderer->Update(m_timer);
	});
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool LitEngineMain::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	// Render the scene objects.
	// TODO: Replace this with your app's content rendering functions.
	return m_sceneRenderer->Render(m_Glenna3DModel);
}

// Updates application state when the window's size changes (e.g. device orientation change)
void LitEngineMain::OnWindowSizeChanged()
{
	// TODO: Replace this with the size-dependent initialization of your app's content.
	m_sceneRenderer->CreateWindowSizeDependentResources();
}

// Notifies the app that it is being suspended.
void LitEngineMain::OnSuspending()
{
	// TODO: Replace this with your app's suspending logic.

	// Process lifetime management may terminate suspended apps at any time, so it is
	// good practice to save any state that will allow the app to restart where it left off.

	m_sceneRenderer->SaveState();

	// If your application uses video memory allocations that are easy to re-create,
	// consider releasing that memory to make it available to other applications.
}

// Notifes the app that it is no longer suspended.
void LitEngineMain::OnResuming()
{
	// TODO: Replace this with your app's resuming logic.
}

// Notifies renderers that device resources need to be released.
void LitEngineMain::OnDeviceRemoved()
{
	// TODO: Save any necessary application or renderer state and release the renderer
	// and its resources which are no longer valid.
	m_sceneRenderer->SaveState();
	m_sceneRenderer = nullptr;
}
