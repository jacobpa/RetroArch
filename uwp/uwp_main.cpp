/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2018 - Krzysztof Haładyn
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "uwp_main.h"
#include <queues/task_queue.h>
#include <retro_timers.h>
#include "../retroarch.h"
#include "../frontend/frontend.h"
#include "../input/input_keymaps.h"
#include "../input/input_driver.h"
#include "../verbosity.h"
#include "uwp_func.h"

#include <ppltasks.h>

using namespace RetroArchUWP;

using namespace concurrency;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::ViewManagement;
using namespace Windows::Devices::Input;
using namespace Windows::System;
using namespace Windows::System::Profile;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

char uwp_dir_install[PATH_MAX_LENGTH];
char uwp_dir_data[PATH_MAX_LENGTH];
char uwp_device_family[128];

// Some keys are unavailable in the VirtualKey enum (wtf) but the old-style constants work
const struct rarch_key_map rarch_key_map_uwp[] = {
   { (unsigned int)VirtualKey::Back, RETROK_BACKSPACE },
   { (unsigned int)VirtualKey::Tab, RETROK_TAB },
   { (unsigned int)VirtualKey::Clear, RETROK_CLEAR },
   { (unsigned int)VirtualKey::Enter, RETROK_RETURN },
   { (unsigned int)VirtualKey::Pause, RETROK_PAUSE },
   { (unsigned int)VirtualKey::Escape, RETROK_ESCAPE },
   { (unsigned int)VirtualKey::ModeChange, RETROK_MODE },
   { (unsigned int)VirtualKey::Space, RETROK_SPACE },
   { (unsigned int)VirtualKey::PageUp, RETROK_PAGEUP },
   { (unsigned int)VirtualKey::PageDown, RETROK_PAGEDOWN },
   { (unsigned int)VirtualKey::End, RETROK_END },
   { (unsigned int)VirtualKey::Home, RETROK_HOME },
   { (unsigned int)VirtualKey::Left, RETROK_LEFT },
   { (unsigned int)VirtualKey::Up, RETROK_UP },
   { (unsigned int)VirtualKey::Right, RETROK_RIGHT },
   { (unsigned int)VirtualKey::Down, RETROK_DOWN },
   { (unsigned int)VirtualKey::Print, RETROK_PRINT },
   { (unsigned int)VirtualKey::Insert, RETROK_INSERT },
   { (unsigned int)VirtualKey::Delete, RETROK_DELETE },
   { (unsigned int)VirtualKey::Help, RETROK_HELP },
   { (unsigned int)VirtualKey::Number0, RETROK_0 },
   { (unsigned int)VirtualKey::Number1, RETROK_1 },
   { (unsigned int)VirtualKey::Number2, RETROK_2 },
   { (unsigned int)VirtualKey::Number3, RETROK_3 },
   { (unsigned int)VirtualKey::Number4, RETROK_4 },
   { (unsigned int)VirtualKey::Number5, RETROK_5 },
   { (unsigned int)VirtualKey::Number6, RETROK_6 },
   { (unsigned int)VirtualKey::Number7, RETROK_7 },
   { (unsigned int)VirtualKey::Number8, RETROK_8 },
   { (unsigned int)VirtualKey::Number9, RETROK_9 },
   { (unsigned int)VirtualKey::A, RETROK_a },
   { (unsigned int)VirtualKey::B, RETROK_b },
   { (unsigned int)VirtualKey::C, RETROK_c },
   { (unsigned int)VirtualKey::D, RETROK_d },
   { (unsigned int)VirtualKey::E, RETROK_e },
   { (unsigned int)VirtualKey::F, RETROK_f },
   { (unsigned int)VirtualKey::G, RETROK_g },
   { (unsigned int)VirtualKey::H, RETROK_h },
   { (unsigned int)VirtualKey::I, RETROK_i },
   { (unsigned int)VirtualKey::J, RETROK_j },
   { (unsigned int)VirtualKey::K, RETROK_k },
   { (unsigned int)VirtualKey::L, RETROK_l },
   { (unsigned int)VirtualKey::M, RETROK_m },
   { (unsigned int)VirtualKey::N, RETROK_n },
   { (unsigned int)VirtualKey::O, RETROK_o },
   { (unsigned int)VirtualKey::P, RETROK_p },
   { (unsigned int)VirtualKey::Q, RETROK_q },
   { (unsigned int)VirtualKey::R, RETROK_r },
   { (unsigned int)VirtualKey::S, RETROK_s },
   { (unsigned int)VirtualKey::T, RETROK_t },
   { (unsigned int)VirtualKey::U, RETROK_u },
   { (unsigned int)VirtualKey::V, RETROK_v },
   { (unsigned int)VirtualKey::W, RETROK_w },
   { (unsigned int)VirtualKey::X, RETROK_x },
   { (unsigned int)VirtualKey::Y, RETROK_y },
   { (unsigned int)VirtualKey::Z, RETROK_z },
   { (unsigned int)VirtualKey::LeftWindows, RETROK_LSUPER },
   { (unsigned int)VirtualKey::RightWindows, RETROK_RSUPER },
   { (unsigned int)VirtualKey::Application, RETROK_MENU },
   { (unsigned int)VirtualKey::NumberPad0, RETROK_KP0 },
   { (unsigned int)VirtualKey::NumberPad1, RETROK_KP1 },
   { (unsigned int)VirtualKey::NumberPad2, RETROK_KP2 },
   { (unsigned int)VirtualKey::NumberPad3, RETROK_KP3 },
   { (unsigned int)VirtualKey::NumberPad4, RETROK_KP4 },
   { (unsigned int)VirtualKey::NumberPad5, RETROK_KP5 },
   { (unsigned int)VirtualKey::NumberPad6, RETROK_KP6 },
   { (unsigned int)VirtualKey::NumberPad7, RETROK_KP7 },
   { (unsigned int)VirtualKey::NumberPad8, RETROK_KP8 },
   { (unsigned int)VirtualKey::NumberPad9, RETROK_KP9 },
   { (unsigned int)VirtualKey::Multiply, RETROK_KP_MULTIPLY },
   { (unsigned int)VirtualKey::Add, RETROK_KP_PLUS },
   { (unsigned int)VirtualKey::Subtract, RETROK_KP_MINUS },
   { (unsigned int)VirtualKey::Decimal, RETROK_KP_PERIOD },
   { (unsigned int)VirtualKey::Divide, RETROK_KP_DIVIDE },
   { (unsigned int)VirtualKey::F1, RETROK_F1 },
   { (unsigned int)VirtualKey::F2, RETROK_F2 },
   { (unsigned int)VirtualKey::F3, RETROK_F3 },
   { (unsigned int)VirtualKey::F4, RETROK_F4 },
   { (unsigned int)VirtualKey::F5, RETROK_F5 },
   { (unsigned int)VirtualKey::F6, RETROK_F6 },
   { (unsigned int)VirtualKey::F7, RETROK_F7 },
   { (unsigned int)VirtualKey::F8, RETROK_F8 },
   { (unsigned int)VirtualKey::F9, RETROK_F9 },
   { (unsigned int)VirtualKey::F10, RETROK_F10 },
   { (unsigned int)VirtualKey::F11, RETROK_F11 },
   { (unsigned int)VirtualKey::F12, RETROK_F12 },
   { (unsigned int)VirtualKey::F13, RETROK_F13 },
   { (unsigned int)VirtualKey::F14, RETROK_F14 },
   { (unsigned int)VirtualKey::F15, RETROK_F15 },
   { (unsigned int)VirtualKey::NumberKeyLock, RETROK_NUMLOCK },
   { (unsigned int)VirtualKey::Scroll, RETROK_SCROLLOCK },
   { (unsigned int)VirtualKey::LeftShift, RETROK_LSHIFT },
   { (unsigned int)VirtualKey::RightShift, RETROK_RSHIFT },
   { (unsigned int)VirtualKey::LeftControl, RETROK_LCTRL },
   { (unsigned int)VirtualKey::RightControl, RETROK_RCTRL },
   { (unsigned int)VirtualKey::LeftMenu, RETROK_LALT },
   { (unsigned int)VirtualKey::RightMenu, RETROK_RALT },
   { VK_RETURN, RETROK_KP_ENTER },
   { (unsigned int)VirtualKey::CapitalLock, RETROK_CAPSLOCK },
   { VK_OEM_1, RETROK_SEMICOLON },
   { VK_OEM_PLUS, RETROK_EQUALS },
   { VK_OEM_COMMA, RETROK_COMMA },
   { VK_OEM_MINUS, RETROK_MINUS },
   { VK_OEM_PERIOD, RETROK_PERIOD },
   { VK_OEM_2, RETROK_SLASH },
   { VK_OEM_3, RETROK_BACKQUOTE },
   { VK_OEM_4, RETROK_LEFTBRACKET },
   { VK_OEM_5, RETROK_BACKSLASH },
   { VK_OEM_6, RETROK_RIGHTBRACKET },
   { VK_OEM_7, RETROK_QUOTE },
   { 0, RETROK_UNKNOWN }
};

struct uwp_input_state_t {
   short mouse_screen_x;
   short mouse_screen_y;
   short mouse_rel_x;
   short mouse_rel_y;
   bool mouse_left;
   bool mouse_right;
   bool mouse_middle;
   bool mouse_button4;
   bool mouse_button5;
   short mouse_wheel_left;
   short mouse_wheel_up;
   short touch_screen_x;
   short touch_screen_y;
   short touch_rel_x;
   short touch_rel_y;
   bool touch_touched;
};

struct uwp_input_state_t uwp_current_input, uwp_next_input;

// The main function is only used to initialize our IFrameworkView class.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	Platform::String^ install_dir = Windows::ApplicationModel::Package::Current->InstalledLocation->Path + L"\\";
	wcstombs(uwp_dir_install, install_dir->Data(), PATH_MAX_LENGTH);
	Platform::String^ data_dir = Windows::Storage::ApplicationData::Current->LocalFolder->Path + L"\\";
	wcstombs(uwp_dir_data, data_dir->Data(), PATH_MAX_LENGTH);

	wcstombs(uwp_device_family, AnalyticsInfo::VersionInfo->DeviceFamily->Data(), 128);

	RARCH_LOG("Data dir: %ls\n", data_dir->Data());
	RARCH_LOG("Install dir: %ls\n", install_dir->Data());

	auto direct3DApplicationSource = ref new Direct3DApplicationSource();
	CoreApplication::Run(direct3DApplicationSource);
	return 0;
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
	return ref new App();
}

App^ App::m_instance;

App::App() :
	m_initialized(false),
	m_windowClosed(false),
	m_windowVisible(true),
	m_windowFocused(true),
	m_windowResized(false)
{
	m_instance = this;
}

// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ applicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &App::OnResuming);
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ window)
{
	window->SizeChanged +=
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnWindowSizeChanged);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);

	window->Activated +=
		ref new TypedEventHandler<CoreWindow^, WindowActivatedEventArgs^>(this, &App::OnWindowActivated);

	window->Closed +=
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);

	window->KeyDown +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKey);

	window->KeyUp +=
		ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &App::OnKey);

	window->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointer);

	window->PointerReleased +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointer);

	window->PointerMoved +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointer);

	window->PointerWheelChanged +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &App::OnPointer);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDpiChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDisplayContentsInvalidated);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnOrientationChanged);

	Windows::UI::Core::SystemNavigationManager::GetForCurrentView()->BackRequested +=
		ref new EventHandler<Windows::UI::Core::BackRequestedEventArgs^>(this, &App::OnBackRequested);
}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(Platform::String^ entryPoint)
{
	int ret = rarch_main(NULL, NULL, NULL);
	if (ret != 0)
	{
		RARCH_ERR("Init failed\n");
		CoreApplication::Exit();
		return;
	}
	m_initialized = true;
}

// This method is called after the window becomes active.
void App::Run()
{
	if (!m_initialized)
	{
		RARCH_WARN("Initialization failed, so not running\n");
		return;
	}

	bool x = false;
	while (true)
	{
		CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

		unsigned sleep_ms = 0;
		int           ret = runloop_iterate(&sleep_ms);

		if (ret == 1 && sleep_ms > 0)
			retro_sleep(sleep_ms);

		task_queue_check();

		if (!x)
		{
			/* HACK: I have no idea why is this necessary but it is required to get proper scaling on Xbox *
			 * Perhaps PreferredLaunchViewSize is broken and we need to wait until the app starts to call TryResizeView */
			m_windowResized = true;
			x = true;
		}

		if (ret == -1)
			break;
	}
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
	main_exit(NULL);
}

// Application lifecycle event handlers.

void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// Run() won't start until the CoreWindow is activated.
	CoreWindow::GetForCurrentThread()->Activate();
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	create_task([this, deferral]()
	{
		// TODO: Maybe creating a save state here would be a good idea?

		deferral->Complete();
	});
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.
}

void App::OnBackRequested(Platform::Object^ sender, Windows::UI::Core::BackRequestedEventArgs^ args)
{
	/* Prevent the B controller button on Xbox One from quitting the app */
	args->Handled = true;
}

// Window event handlers.

void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	m_windowResized = true;
}

void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void App::OnWindowActivated(CoreWindow^ sender, WindowActivatedEventArgs^ args)
{
	m_windowFocused = args->WindowActivationState != CoreWindowActivationState::Deactivated;
}

void App::OnKey(CoreWindow^ sender, KeyEventArgs^ args)
{
	uint16_t mod = 0;
	if ((sender->GetKeyState(VirtualKey::Shift) & CoreVirtualKeyStates::Locked) == CoreVirtualKeyStates::Locked)
		mod |= RETROKMOD_SHIFT;
	if ((sender->GetKeyState(VirtualKey::Control) & CoreVirtualKeyStates::Locked) == CoreVirtualKeyStates::Locked)
		mod |= RETROKMOD_CTRL;
	if ((sender->GetKeyState(VirtualKey::Menu) & CoreVirtualKeyStates::Locked) == CoreVirtualKeyStates::Locked)
		mod |= RETROKMOD_ALT;
	if ((sender->GetKeyState(VirtualKey::CapitalLock) & CoreVirtualKeyStates::Locked) == CoreVirtualKeyStates::Locked)
		mod |= RETROKMOD_CAPSLOCK;
	if ((sender->GetKeyState(VirtualKey::Scroll) & CoreVirtualKeyStates::Locked) == CoreVirtualKeyStates::Locked)
		mod |= RETROKMOD_SCROLLOCK;
	if ((sender->GetKeyState(VirtualKey::LeftWindows) & CoreVirtualKeyStates::Locked) == CoreVirtualKeyStates::Locked ||
		(sender->GetKeyState(VirtualKey::RightWindows) & CoreVirtualKeyStates::Locked) == CoreVirtualKeyStates::Locked)
		mod |= RETROKMOD_META;

	unsigned keycode = input_keymaps_translate_keysym_to_rk((unsigned)args->VirtualKey);

	input_keyboard_event(!args->KeyStatus.IsKeyReleased, keycode, 0, mod, RETRO_DEVICE_KEYBOARD);
}

void App::OnPointer(CoreWindow^ sender, PointerEventArgs^ args)
{
	if (args->CurrentPoint->PointerDevice->PointerDeviceType == PointerDeviceType::Mouse)
	{
		uwp_next_input.mouse_left = args->CurrentPoint->Properties->IsLeftButtonPressed;
		uwp_next_input.mouse_middle = args->CurrentPoint->Properties->IsMiddleButtonPressed;
		uwp_next_input.mouse_right = args->CurrentPoint->Properties->IsRightButtonPressed;
		uwp_next_input.mouse_button4 = args->CurrentPoint->Properties->IsXButton1Pressed;
		uwp_next_input.mouse_button5 = args->CurrentPoint->Properties->IsXButton2Pressed;
		uwp_next_input.mouse_screen_x = args->CurrentPoint->Position.X;
		uwp_next_input.mouse_screen_y = args->CurrentPoint->Position.Y;
		uwp_next_input.mouse_rel_x = uwp_next_input.mouse_screen_x - uwp_current_input.mouse_screen_x;
		uwp_next_input.mouse_rel_y = uwp_next_input.mouse_screen_y - uwp_current_input.mouse_screen_y;
		if (args->CurrentPoint->Properties->IsHorizontalMouseWheel)
			uwp_next_input.mouse_wheel_left += args->CurrentPoint->Properties->MouseWheelDelta;
		else
			uwp_next_input.mouse_wheel_up += args->CurrentPoint->Properties->MouseWheelDelta;
	}
	else
	{
		uwp_next_input.touch_touched = args->CurrentPoint->IsInContact;
		uwp_next_input.touch_screen_x = args->CurrentPoint->Position.X;
		uwp_next_input.touch_screen_y = args->CurrentPoint->Position.Y;
		uwp_next_input.touch_rel_x = uwp_next_input.touch_screen_x - uwp_current_input.touch_screen_x;
		uwp_next_input.touch_rel_y = uwp_next_input.touch_screen_y - uwp_current_input.touch_screen_y;
	}
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

// DisplayInformation event handlers.

void App::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
	m_windowResized = true;
}

void App::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	m_windowResized = true;
}

void App::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
	// Probably can be ignored?
}

// Taken from DirectX UWP samples - on Xbox, everything is scaled 200% so getting the DPI calculation correct is crucial
static inline float ConvertDipsToPixels(float dips, float dpi)
{
	static const float dipsPerInch = 96.0f;
	return floorf(dips * dpi / dipsPerInch + 0.5f);
}

// Implement UWP equivalents of various win32_* functions
extern "C" {

	bool win32_has_focus(void)
	{
		return App::GetInstance()->IsWindowFocused();
	}

	bool win32_set_video_mode(void *data, unsigned width, unsigned height, bool fullscreen)
	{
		if (App::GetInstance()->IsInitialized())
		{
			if (fullscreen != ApplicationView::GetForCurrentView()->IsFullScreenMode)
			{
				if (fullscreen)
					ApplicationView::GetForCurrentView()->TryEnterFullScreenMode();
				else
					ApplicationView::GetForCurrentView()->ExitFullScreenMode();
			}
			ApplicationView::GetForCurrentView()->TryResizeView(Size(width, height));
		}
		else
		{
			/* In case the window is not activated yet, TryResizeView will fail and we have to set the initial parameters instead */
			/* Note that these are preserved after restarting the app and used for the UWP splash screen size (!), so they should be set only during init and not changed afterwards */
			ApplicationView::PreferredLaunchViewSize = Size(width, height);
			ApplicationView::PreferredLaunchWindowingMode = fullscreen ? ApplicationViewWindowingMode::FullScreen : ApplicationViewWindowingMode::PreferredLaunchViewSize;
		}

		/* Setting the window size may sometimes fail "because UWP"
		 * (i.e. we are on device with no windows, or Windows sandbox decides the window can't be that small)
		 * so in case resizing fails we just send a resized event back to RetroArch with old size
		 * (and report success because otherwise it bails out hard about failing to set video mode)
		 */
		App::GetInstance()->SetWindowResized();
		return true;
	}

	void win32_show_cursor(bool state)
	{
		CoreWindow::GetForCurrentThread()->PointerCursor = state ? ref new CoreCursor(CoreCursorType::Arrow, 0) : nullptr;
	}

	void win32_check_window(bool *quit, bool *resize, unsigned *width, unsigned *height)
	{
		*quit   = App::GetInstance()->IsWindowClosed();
		*resize = App::GetInstance()->CheckWindowResized();
		if (*resize)
		{
			float dpi = DisplayInformation::GetForCurrentView()->LogicalDpi;
			*width    = ConvertDipsToPixels(CoreWindow::GetForCurrentThread()->Bounds.Width, dpi);
			*height   = ConvertDipsToPixels(CoreWindow::GetForCurrentThread()->Bounds.Height, dpi);
		}
	}

	void* uwp_get_corewindow(void)
	{
		return (void*)CoreWindow::GetForCurrentThread();
	}

	void uwp_input_next_frame(void)
	{
		uwp_current_input                = uwp_next_input;
		uwp_next_input.mouse_rel_x       = 0;
		uwp_next_input.mouse_rel_y       = 0;
		uwp_next_input.mouse_wheel_up   %= WHEEL_DELTA;
		uwp_next_input.mouse_wheel_left %= WHEEL_DELTA;
		uwp_next_input.touch_rel_x       = 0;
		uwp_next_input.touch_rel_y       = 0;
	}

	bool uwp_keyboard_pressed(unsigned key)
	{
		unsigned sym = rarch_keysym_lut[(enum retro_key)key];
		return (CoreWindow::GetForCurrentThread()->GetKeyState((VirtualKey)sym) & CoreVirtualKeyStates::Down) == CoreVirtualKeyStates::Down;
	}

	int16_t uwp_mouse_state(unsigned port, unsigned id, bool screen)
	{
		int16_t state = 0;

		switch (id)
		{
		case RETRO_DEVICE_ID_MOUSE_X:
			return screen ? uwp_current_input.mouse_screen_x : uwp_current_input.mouse_rel_x;
		case RETRO_DEVICE_ID_MOUSE_Y:
			return screen ? uwp_current_input.mouse_screen_y : uwp_current_input.mouse_rel_y;
		case RETRO_DEVICE_ID_MOUSE_LEFT:
			return uwp_current_input.mouse_left;
		case RETRO_DEVICE_ID_MOUSE_RIGHT:
			return uwp_current_input.mouse_right;
		case RETRO_DEVICE_ID_MOUSE_WHEELUP:
			return uwp_current_input.mouse_wheel_up > WHEEL_DELTA;
		case RETRO_DEVICE_ID_MOUSE_WHEELDOWN:
			return uwp_current_input.mouse_wheel_up < -WHEEL_DELTA;
		case RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP:
			return uwp_current_input.mouse_wheel_left > WHEEL_DELTA;
		case RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN:
			return uwp_current_input.mouse_wheel_left < -WHEEL_DELTA;
		case RETRO_DEVICE_ID_MOUSE_MIDDLE:
			return uwp_current_input.mouse_middle;
		case RETRO_DEVICE_ID_MOUSE_BUTTON_4:
			return uwp_current_input.mouse_button4;
		case RETRO_DEVICE_ID_MOUSE_BUTTON_5:
			return uwp_current_input.mouse_button5;
		}

		return 0;
	}

	// TODO: I don't have any touch-enabled Windows devices to test if this actually works
	int16_t uwp_pointer_state(unsigned idx, unsigned id, bool screen)
	{
		switch (id)
		{
		case RETRO_DEVICE_ID_POINTER_X:
			return screen ? uwp_current_input.touch_screen_x : uwp_current_input.touch_rel_x;
		case RETRO_DEVICE_ID_POINTER_Y:
			return screen ? uwp_current_input.touch_screen_y : uwp_current_input.touch_rel_y;
		case RETRO_DEVICE_ID_POINTER_PRESSED:
			return uwp_current_input.touch_touched;
		default:
			break;
		}

		return 0;
	}
}