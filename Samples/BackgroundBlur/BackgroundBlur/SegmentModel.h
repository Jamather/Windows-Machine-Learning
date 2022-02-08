#pragma once
#include <winrt/Microsoft.AI.MachineLearning.Experimental.h>
#include <winrt/Microsoft.AI.MachineLearning.h>
#include <Windows.AI.MachineLearning.native.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.media.core.interop.h>
#include <strsafe.h>
#include <wtypes.h>
#include <winrt/base.h>
#include <dxgi.h>
#include <d3d11.h>
#include <mutex>
#include <winrt/windows.foundation.collections.h>
#include <winrt/Windows.Media.h>

using namespace winrt::Microsoft::AI::MachineLearning;
using namespace winrt::Microsoft::AI::MachineLearning::Experimental;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Media;

// Model-agnostic helper LearningModels
LearningModel Normalize0_1ThenZScore(long height, long width, long channels, const std::array<float, 3>& means, const std::array<float, 3>& stddev);
LearningModel ReshapeFlatBufferToNCHW(long n, long c, long h, long w);
LearningModel Invert(long n, long c, long h, long w);

class IStreamModel
{
public:
	IStreamModel(): 
		m_inputVideoFrame(NULL),
		m_outputVideoFrame(NULL),
		m_session(NULL),
		m_binding(NULL)
	{}
	IStreamModel(int w, int h) :
		m_inputVideoFrame(NULL),
		m_outputVideoFrame(NULL),
		m_session(NULL),
		m_binding(NULL)
	{}
	~IStreamModel() {
		if(m_session) m_session.Close();
		if(m_binding) m_binding.Clear();
	};
	virtual void SetModels(int w, int h) =0;
	virtual void Run(IDirect3DSurface src, IDirect3DSurface dest) =0;

	void SetUseGPU(bool use) { 
		m_bUseGPU = use;
	}
	void SetDevice() {
		m_device = m_session.Device().Direct3D11Device();
	}

protected:
	void SetVideoFrames(VideoFrame inVideoFrame, VideoFrame outVideoFrame) 
	{
		if (true || !m_bVideoFramesSet)
		{
			if (m_device == NULL)
			{
				SetDevice();
			}
			auto inDesc = inVideoFrame.Direct3DSurface().Description();
			auto outDesc = outVideoFrame.Direct3DSurface().Description();
			/*
				NOTE: VideoFrame::CreateAsDirect3D11SurfaceBacked takes arguments in (width, height) order
				whereas every model created with LearningModelBuilder takes arguments in (height, width) order. 
			*/ 
			m_inputVideoFrame = VideoFrame::CreateAsDirect3D11SurfaceBacked(inDesc.Format, m_imageWidthInPixels, m_imageHeightInPixels, m_device);
			m_outputVideoFrame = VideoFrame::CreateAsDirect3D11SurfaceBacked(outDesc.Format, m_imageWidthInPixels, m_imageHeightInPixels, m_device);
			m_bVideoFramesSet = true;
		}
		// TODO: Fix bug in WinML so that the surfaces from capture engine are shareable, remove copy. 
		inVideoFrame.CopyToAsync(m_inputVideoFrame).get();
		outVideoFrame.CopyToAsync(m_outputVideoFrame).get();
	}

	void SetImageSize(int w, int h) {
		m_imageWidthInPixels = w;
		m_imageHeightInPixels = h;
	}

	LearningModelSession CreateLearningModelSession(const LearningModel& model, bool closedModel = true) {
		auto device = m_bUseGPU ? LearningModelDevice(LearningModelDeviceKind::DirectXHighPerformance) : LearningModelDevice(LearningModelDeviceKind::Default);
		auto options = LearningModelSessionOptions();
		options.BatchSizeOverride(0);
		options.CloseModelOnSessionCreation(closedModel);
		auto session = LearningModelSession(model, device, options);
		return session;
	}
	bool						m_bUseGPU = true;
	bool						m_bVideoFramesSet = false;
	VideoFrame					m_inputVideoFrame,
								m_outputVideoFrame;
	UINT32                      m_imageWidthInPixels;
	UINT32                      m_imageHeightInPixels;
	IDirect3DDevice				m_device;

	// Learning Model Binding and Session. 
	// Derived classes can add more as needed for chaining? 
	LearningModelSession m_session;
	LearningModelBinding m_binding;

}; 

// TODO: Make an even more Invert IStreamModel? 

class StyleTransfer : public IStreamModel {
public:
	StyleTransfer(int w, int h) : IStreamModel(w, h) {
		SetModels(w, h); }
	StyleTransfer() : IStreamModel() {};
	void SetModels(int w, int h);
	void Run(IDirect3DSurface src, IDirect3DSurface dest);
private: 
	LearningModel GetModel();
};


class BackgroundBlur : public IStreamModel
{
public:
	BackgroundBlur(int w, int h) : 
		IStreamModel(w, h), 
		m_sessionPreprocess(NULL),
		m_sessionPostprocess(NULL),
		m_bindingPreprocess(NULL),
		m_bindingPostprocess(NULL)
	{
		SetModels(w, h);
	}
	BackgroundBlur() : 
		IStreamModel(),
		m_sessionPreprocess(NULL),
		m_sessionPostprocess(NULL),
		m_bindingPreprocess(NULL),
		m_bindingPostprocess(NULL)
	{};
	void SetModels(int w, int h);
	void Run(IDirect3DSurface src, IDirect3DSurface dest);
private:
	LearningModel GetModel();
	LearningModel PostProcess(long n, long c, long h, long w, long axis);

	// Background blur-specific sessions, bindings 
	LearningModelSession m_sessionPreprocess; 
	LearningModelSession m_sessionPostprocess; 
	LearningModelBinding m_bindingPreprocess;
	LearningModelBinding m_bindingPostprocess; 
};