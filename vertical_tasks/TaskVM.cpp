#include "pch.h"
#include "TaskVM.h"
#include "TaskVM.g.cpp"

#include <iostream>
#include <sstream>


#include "imagehelper.h"
#include <winrt/microsoft.ui.xaml.media.imaging.h>
#include <appmodel.h>
#include <winuser.h>
#include <knownfolders.h>
#include <shobjidl_core.h>
#include <shlobj_core.h>
#include <windows.graphics.imaging.h>
#include <windows.graphics.imaging.interop.h>

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Graphics::Imaging;
}

namespace winui
{
    using namespace winrt::Microsoft::UI::Xaml;
    using namespace winrt::Microsoft::UI::Xaml::Media;
    using namespace winrt::Microsoft::UI::Xaml::Media::Imaging;
}

namespace winrt::vertical_tasks::implementation
{
    winrt::fire_and_forget TaskVM::RefreshTitleAndIcon(bool update)
    {
        // recalc m_procName if we are applicationframehost

        winrt::hstring newTitle = m_window.GetTitle();

        auto weak_ref{ get_weak() };
        co_await winrt::resume_background();

        auto strong_ref{ weak_ref.get() };

        auto maybeIcon = m_window.TryGetIconFromWindow();

        winrt::com_ptr<IWICBitmap> wicBitmap;
        if (maybeIcon)
        {
            // win32 apps usually provide icons from the window
            auto imagingFactory = wil::CoCreateInstance<IWICImagingFactory>(CLSID_WICImagingFactory);
            THROW_IF_FAILED(imagingFactory->CreateBitmapFromHICON(maybeIcon.get(), wicBitmap.put()));
        }
        else
        {
            if (m_window.Aumid().empty())
            {
                // win32 app without an icon from the window, expected
                auto nameToPrint = m_window.ProcessName();
                LOG_HR_MSG(E_INVALIDARG, "%ws has no icon", m_window.ProcessName());
            }
            else
            {
                // uwp app, so use aumid for lookup
                wil::com_ptr<IShellItemImageFactory> imageFactory;
                if (SUCCEEDED_LOG(SHCreateItemInKnownFolder(FOLDERID_AppsFolder, KF_FLAG_DONT_VERIFY, m_window.Aumid().c_str(), IID_PPV_ARGS(&imageFactory))))
                {
                    wil::unique_hbitmap iconBitmap;

                    THROW_IF_FAILED(imageFactory->GetImage(m_iconSize, SIIGBF_BIGGERSIZEOK, &iconBitmap));

                    auto imagingFactory = wil::CoCreateInstance<IWICImagingFactory>(CLSID_WICImagingFactory);
                    THROW_IF_FAILED(imagingFactory->CreateBitmapFromHBITMAP(iconBitmap.get(), nullptr, WICBitmapUsePremultipliedAlpha, wicBitmap.put()));
                }
            }
        }

        winrt::SoftwareBitmap bitmap{ nullptr };
        if (wicBitmap)
        {
            auto bitmapFactory = wil::CoCreateInstance<ISoftwareBitmapNativeFactory>(CLSID_SoftwareBitmapNativeFactory);
            winrt::com_ptr<ISoftwareBitmapNative> nativeBitmap;
            THROW_IF_FAILED(bitmapFactory->CreateFromWICBitmap(wicBitmap.get(), false, IID_PPV_ARGS(nativeBitmap.put())));

            bitmap = wil::convert_from_abi<winrt::SoftwareBitmap>(nativeBitmap.get());
            if (bitmap.BitmapPixelFormat() != winrt::BitmapPixelFormat::Bgra8 || bitmap.BitmapAlphaMode() != winrt::BitmapAlphaMode::Premultiplied)
            {
                bitmap = winrt::SoftwareBitmap::Convert(bitmap, winrt::BitmapPixelFormat::Bgra8, winrt::BitmapAlphaMode::Premultiplied);
            }
        }

        const bool hasChanges = (bitmap || (m_title != newTitle));
        if (update && hasChanges)
        {
            if (auto queue = m_uiThread.get())
            {
                co_await wil::resume_foreground(queue);

                // send change notification
                if (m_title != newTitle)
                {
                    m_title = newTitle;
                    OnPropertyChanged(L"Title");
                }

                if (bitmap)
                {
                    winui::SoftwareBitmapSource source{};
                    co_await source.SetBitmapAsync(bitmap);
                    IconSource(source);
                }

            }
        }
    }

    void TaskVM::Select()   { Print(L"Select");         m_window.Select(); }
    void TaskVM::Close()    { Print(L"Close");          m_window.Close(); };
    void TaskVM::Kill()     { Print(L"Kill");           m_window.Kill(); };
    void TaskVM::Minimize() { Print(L"Minimize");       m_window.Minimize(); }

    void TaskVM::Print()
    {
        Print(L"");
    }

    void TaskVM::Print(std::wstring_view category)
    {
        std::wstringstream myString;
        auto hwnd = m_window.HWND();
        myString << L"\tWindow: " << std::hex << hwnd;
        myString << L"\tProc: " << m_window.ProcessName() << std::endl;
        if (category.empty())
        {
            myString << L"\t Cloak State: " << m_window.IsCloaked() << std::endl;
        }
        else
        {
            myString << category << std::endl;
        }
        OutputDebugString(myString.str().c_str());
    }
}
