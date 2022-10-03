#pragma once
#include "pch.h"
#include <wincodec.h>
#include <assert.h>
#include <shlwapi.h>
#include <shcore.h>

#include <wil/com.h>
#include <wil/resource.h>
#include <winrt/windows.graphics.imaging.h>

inline HRESULT SHCreateMemoryStream(_In_reads_bytes_opt_(cbInit) const BYTE* pbInit, UINT cbInit, _COM_Outptr_ IStream** ppstm)
{
    *ppstm = SHCreateMemStream(pbInit, cbInit);
    return (*ppstm) ? S_OK : E_OUTOFMEMORY;
}

// Most common pixel format - GUID_WICPixelFormat32bppPBGRA
// Most common dither type - WICBitmapDitherTypeNone
inline HRESULT ConvertWICBitmapPixelFormat(_In_ IWICImagingFactory* pWICImagingFactory, _In_ IWICBitmapSource* pWICBitmapSource, _In_ WICPixelFormatGUID guidPixelFormatSource, _In_ WICBitmapDitherType bitmapDitherType, _COM_Outptr_ IWICBitmapSource** ppWICBitmapSource)
{
    *ppWICBitmapSource = nullptr;

    wil::com_ptr<IWICFormatConverter> spWICFormatConverter;
    HRESULT hr = pWICImagingFactory->CreateFormatConverter(&spWICFormatConverter);
    if (SUCCEEDED(hr))
    {
        hr = spWICFormatConverter->Initialize(pWICBitmapSource, guidPixelFormatSource, bitmapDitherType, nullptr, 0.f, WICBitmapPaletteTypeCustom);
        // Store the converted bitmap as ppToRenderBitmapSource 
        if (SUCCEEDED(hr))
        {
            spWICFormatConverter.copy_to(IID_PPV_ARGS(ppWICBitmapSource));
        }
    }
    return hr;
}

enum class JpegQuantizationTableTypes
{
    PhotoshopSaveAs09 = 0
};

// JPEG Quantization Tables can be used in place of the ImageQuality encoder option to affect
// the size and quality of compressed images.  The data here is publicly available and
// referenced from: http://www.impulseadventure.com/photo/jpeg-quantization.html.
struct SJpegQuantizationTable
{
    JpegQuantizationTableTypes TableType;
    INT32 const Luminance[64];
    INT32 const Chrominance[64];
};
extern __declspec(selectany) SJpegQuantizationTable const JpegQuantizationTables[] =
{
    {
        JpegQuantizationTableTypes::PhotoshopSaveAs09,
        {
            4, 3, 4, 7, 9, 11, 14, 17,
            3, 3, 4, 7, 9, 12, 12, 12,
            4, 4, 5, 9, 12, 12, 12, 12,
            7, 7, 9, 12, 12, 12, 12, 12,
            9, 9, 12, 12, 12, 12, 12, 12,
            11, 12, 12, 12, 12, 12, 12, 12,
            14, 12, 12, 12, 12, 12, 12, 12,
            17, 12, 12, 12, 12, 12, 12, 12
        },
        {
            4, 6, 12, 22, 20, 20, 17, 17,
            6, 8, 12, 14, 14, 12, 12, 12,
            12, 12, 14, 14, 12, 12, 12, 12,
            22, 14, 14, 12, 12, 12, 12, 12,
            20, 14, 12, 12, 12, 12, 12, 12,
            20, 12, 12, 12, 12, 12, 12, 12,
            17, 12, 12, 12, 12, 12, 12, 12,
            17, 12, 12, 12, 12, 12, 12, 12
        }
    },
};

inline HRESULT SetJpegQuantizationTableOptions(_In_ JpegQuantizationTableTypes tableType, _In_ IPropertyBag2* pEncoderOptions)
{
    DWORD dwIndex = static_cast<DWORD>(tableType);
    HRESULT hr = (dwIndex < ARRAYSIZE(JpegQuantizationTables)) && (JpegQuantizationTables[dwIndex].TableType == tableType) ? S_OK : E_UNEXPECTED;
    if (SUCCEEDED(hr))
    {
        PROPBAG2 options[2] = {};
        options[0].pstrName = const_cast<wchar_t*>(L"Luminance");
        options[0].vt = VT_ARRAY | VT_I4;
        options[1].pstrName = const_cast<wchar_t*>(L"Chrominance");
        options[1].vt = VT_ARRAY | VT_I4;

        VARIANT values[2] = {};
        SAFEARRAYBOUND saBoundLuminace = { ARRAYSIZE(JpegQuantizationTables[dwIndex].Luminance), 0 };
        values[0].parray = SafeArrayCreate(VT_I4, 1, &saBoundLuminace);
        values[0].vt = VT_ARRAY | VT_I4;
        SAFEARRAYBOUND saBoundChrominance = { ARRAYSIZE(JpegQuantizationTables[dwIndex].Chrominance), 0 };
        values[1].parray = SafeArrayCreate(VT_I4, 1, &saBoundChrominance);
        values[1].vt = VT_ARRAY | VT_I4;

        hr = ((values[0].parray != nullptr) && (values[1].parray != nullptr)) ? S_OK : E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            INT32* pData;
            hr = SafeArrayAccessData(values[0].parray, reinterpret_cast<void**>(&pData));
            if (SUCCEEDED(hr))
            {
                memcpy_s(pData, sizeof(JpegQuantizationTables[dwIndex].Luminance), JpegQuantizationTables[dwIndex].Luminance, sizeof(JpegQuantizationTables[dwIndex].Luminance));
                hr = SafeArrayUnaccessData(values[0].parray);
            }
            if (SUCCEEDED(hr))
            {
                hr = SafeArrayAccessData(values[1].parray, reinterpret_cast<void**>(&pData));
                if (SUCCEEDED(hr))
                {
                    memcpy_s(pData, sizeof(JpegQuantizationTables[dwIndex].Chrominance), JpegQuantizationTables[dwIndex].Chrominance, sizeof(JpegQuantizationTables[dwIndex].Chrominance));
                    hr = SafeArrayUnaccessData(values[1].parray);
                }
            }
            if (SUCCEEDED(hr))
            {
                assert(ARRAYSIZE(options) == ARRAYSIZE(values));
                hr = pEncoderOptions->Write(ARRAYSIZE(options), options, values);
            }
        }

        if (values[0].parray != nullptr)
        {
            SafeArrayDestroy(values[0].parray);
        }

        if (values[1].parray != nullptr)
        {
            SafeArrayDestroy(values[1].parray);
        }
    }
    return hr;
}

enum class EncodingOptions : UINT
{
    None = 0x0,
    UseBitmapVersion5 = 0x1,
    UsePhotoshopSaveAs09ForJPEG = 0x2,
};
DEFINE_ENUM_FLAG_OPERATORS(EncodingOptions);

inline HRESULT AddFrameToWICBitmap(_In_ IWICImagingFactory* pWICImagingFactory, _In_ IWICBitmapEncoder* pWICBitmapEncoder, _In_ IWICBitmapSource* pWICBitmapSource, WICPixelFormatGUID pixelFormat, _In_ EncodingOptions options)
{
    wil::com_ptr<IWICBitmapFrameEncode> spWICFrameEncoder;
    wil::com_ptr<IPropertyBag2> spWICEncoderOptions;

    HRESULT hr = pWICBitmapEncoder->CreateNewFrame(&spWICFrameEncoder, &spWICEncoderOptions);
    if (SUCCEEDED(hr))
    {
        GUID containerGuid;
        hr = pWICBitmapEncoder->GetContainerFormat(&containerGuid);
        if (SUCCEEDED(hr))
        {
            if ((containerGuid == GUID_ContainerFormatBmp) && WI_IsFlagSet(options, EncodingOptions::UseBitmapVersion5))
            {
                // Write the encoder option to the property bag instance.
                VARIANT varValue = {};
                PROPBAG2 v5HeaderOption = {};

                // Options to enable the v5 header support for 32bppBGRA.
                varValue.vt = VT_BOOL;
                varValue.boolVal = VARIANT_TRUE;
                v5HeaderOption.pstrName = const_cast<wchar_t*>(L"EnableV5Header32bppBGRA");

                hr = spWICEncoderOptions->Write(1, &v5HeaderOption, &varValue);
            }
            else if ((containerGuid == GUID_ContainerFormatJpeg) && WI_IsFlagSet(options, EncodingOptions::UsePhotoshopSaveAs09ForJPEG))
            {
                hr = SetJpegQuantizationTableOptions(JpegQuantizationTableTypes::PhotoshopSaveAs09, spWICEncoderOptions.get());
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = spWICFrameEncoder->Initialize(spWICEncoderOptions.get());
            if (SUCCEEDED(hr))
            {
                // Get/set the size of the image
                UINT uWidth, uHeight;
                hr = pWICBitmapSource->GetSize(&uWidth, &uHeight);
                if (SUCCEEDED(hr))
                {
                    hr = spWICFrameEncoder->SetSize(uWidth, uHeight);
                    if (SUCCEEDED(hr))
                    {
                        WICPixelFormatGUID pixelFormatEncoder = pixelFormat;
                        hr = spWICFrameEncoder->SetPixelFormat(&pixelFormatEncoder);
                        if (SUCCEEDED(hr))
                        {
                            wil::com_ptr<IWICBitmapSource> spBitmapSourceConverted;
                            if (pixelFormatEncoder != pixelFormat)
                            {
                                // the encoder doesn't support the format so use a converter
                                hr = ConvertWICBitmapPixelFormat(pWICImagingFactory, pWICBitmapSource, pixelFormat, WICBitmapDitherTypeNone, &spBitmapSourceConverted);
                            }
                            else
                            {
                                spBitmapSourceConverted = pWICBitmapSource;
                            }

                            if (SUCCEEDED(hr))
                            {
                                WICRect rect = { 0, 0, static_cast<INT>(uWidth), static_cast<INT>(uHeight) };
                                // Write the image data and commit
                                hr = spWICFrameEncoder->WriteSource(spBitmapSourceConverted.get(), &rect);
                                if (SUCCEEDED(hr))
                                {
                                    hr = spWICFrameEncoder->Commit();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}

inline HRESULT GetStreamOfWICBitmapSourceWithOptions(_In_opt_ IWICImagingFactory* pWICImagingFactory, _In_ IWICBitmapSource* pWICBitmapSource, _In_ REFGUID guidContainerFormat, _In_ WICPixelFormatGUID pixelFormat, _In_ EncodingOptions options, _COM_Outptr_ IStream** ppStreamOut)
{
    *ppStreamOut = nullptr;

    HRESULT hr = S_OK;
    wil::com_ptr<IWICImagingFactory> spWICImagingFactory = pWICImagingFactory;
    if (spWICImagingFactory == nullptr)
    {
        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&spWICImagingFactory));
    }
    if (SUCCEEDED(hr))
    {
        wil::com_ptr<IStream> spImageStream; //{ SHCreateMemStream(nullptr, 0) }
        hr = SHCreateMemoryStream(nullptr, 0, &spImageStream);
        if (SUCCEEDED(hr))
        {
            // Create encoder and initialize it
            wil::com_ptr<IWICBitmapEncoder> spWICEncoder;
            hr = spWICImagingFactory->CreateEncoder(guidContainerFormat, &GUID_VendorMicrosoft, &spWICEncoder);
            if (SUCCEEDED(hr))
            {
                hr = spWICEncoder->Initialize(spImageStream.get(), WICBitmapEncoderNoCache);
                if (SUCCEEDED(hr))
                {
                    // Add a single frame to the encoder with the Bitmap
                    hr = AddFrameToWICBitmap(spWICImagingFactory.get(), spWICEncoder.get(), pWICBitmapSource, pixelFormat, options);
                    if (SUCCEEDED(hr))
                    {
                        hr = spWICEncoder->Commit();
                        if (SUCCEEDED(hr))
                        {
                            // Seek the stream to the beginning and transfer
                            static const LARGE_INTEGER lnBeginning = {};
                            hr = spImageStream->Seek(lnBeginning, STREAM_SEEK_SET, nullptr);
                            if (SUCCEEDED(hr))
                            {
                                spImageStream.copy_to(ppStreamOut);
                            }
                        }
                    }
                }
            }
        }
    }
    return hr;
}

inline HRESULT GetStreamOfWICBitmapSource(_In_opt_ IWICImagingFactory* pWICImagingFactory, _In_ IWICBitmapSource* pWICBitmapSource, _In_ REFGUID guidContainerFormat, _COM_Outptr_ IStream** ppStreamOut)
{
    return GetStreamOfWICBitmapSourceWithOptions(pWICImagingFactory, pWICBitmapSource, guidContainerFormat, GUID_WICPixelFormat32bppBGRA, EncodingOptions::None, ppStreamOut);
}

//Windows::Foundation::
winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Graphics::Imaging::SoftwareBitmap> GetBitmapFromIconFileAsync(wil::unique_hicon hicon)
{
    // TODO global decoder
    wil::com_ptr<IWICImagingFactory> wicImagingFactory;
    THROW_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wicImagingFactory)));

    wil::com_ptr<IWICBitmap> wicBitmap;
    THROW_IF_FAILED(wicImagingFactory->CreateBitmapFromHICON(hicon.get(), &wicBitmap));

    wil::com_ptr<IWICBitmapSource> wicBitmapSource;
    wicBitmap.query_to(&wicBitmapSource);
    THROW_HR_IF_NULL(E_UNEXPECTED, wicBitmapSource);

    wil::com_ptr<IStream> stream;
    THROW_IF_FAILED(GetStreamOfWICBitmapSource(wicImagingFactory.get(), wicBitmapSource.get(), GUID_ContainerFormatPng, &stream));

    winrt::Windows::Storage::Streams::IRandomAccessStream randomAccessStream{ nullptr };
    THROW_IF_FAILED(CreateRandomAccessStreamOverStream(stream.get(), BSOS_DEFAULT, 
        winrt::guid_of<winrt::Windows::Storage::Streams::IRandomAccessStream>(), winrt::put_abi(randomAccessStream)));

    auto decoder = co_await winrt::Windows::Graphics::Imaging::BitmapDecoder::CreateAsync(randomAccessStream);
    co_return co_await decoder.GetSoftwareBitmapAsync();
}