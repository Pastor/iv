#include <iostream>
#include <iomanip>
#include <urlmon.h>
#include <windows.h>
#include <WinInet.h>

#pragma  comment(lib, "urlmon.lib")
#pragma  comment(lib, "wininet.lib")

void LoadBar(unsigned curr_val, unsigned max_val, unsigned bar_width = 20)
{
    if ((curr_val != max_val) && (curr_val % (max_val / 100) != 0))
        return;

    auto ratio = curr_val / static_cast<double>(max_val);
    auto bar_now = static_cast<unsigned>(ratio * bar_width);

    std::cout << "\r" << std::setw(3) << static_cast<unsigned>(ratio * 100.0) << "% [";
    for (unsigned i = 0; i < bar_now; ++i)
        std::cout << "=";
    for (auto i = bar_now; i < bar_width; ++i)
        std::cout << " ";
    std::cout << "]" << std::flush;
}

class CallbackHandler : public IBindStatusCallback
{
    int m_percentLast;

public:
    virtual ~CallbackHandler()
    {}

    CallbackHandler() : m_percentLast(0)
    {}

    // IUnknown

    HRESULT STDMETHODCALLTYPE
        QueryInterface(REFIID riid, void** ppvObject) override
    {

        if (IsEqualIID(IID_IBindStatusCallback, riid)
            || IsEqualIID(IID_IUnknown, riid)) {
            *ppvObject = reinterpret_cast<void*>(this);
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE
        AddRef() override
    {
        return 2UL;
    }

    ULONG STDMETHODCALLTYPE
        Release() override
    {
        return 1UL;
    }

    // IBindStatusCallback

    HRESULT STDMETHODCALLTYPE
        OnStartBinding(DWORD     /*dwReserved*/,
            IBinding* /*pib*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
        GetPriority(LONG* /*pnPriority*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
        OnLowResource(DWORD /*reserved*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
        OnProgress(ULONG   ulProgress,
            ULONG   ulProgressMax,
            ULONG   ulStatusCode,
            LPCWSTR /*szStatusText*/) override
    {
        switch (ulStatusCode) {
        case BINDSTATUS_FINDINGRESOURCE:
            std::cout << "Finding resource..." << std::endl;
            break;
        case BINDSTATUS_CONNECTING:
            std::cout << "Connecting..." << std::endl;
            break;
        case BINDSTATUS_SENDINGREQUEST:
            std::cout << "Sending request..." << std::endl;
            break;
        case BINDSTATUS_MIMETYPEAVAILABLE:
            std::cout << "Mime type available" << std::endl;
            break;
        case BINDSTATUS_CACHEFILENAMEAVAILABLE:
            std::cout << "Cache filename available" << std::endl;
            break;
        case BINDSTATUS_BEGINDOWNLOADDATA:
            std::cout << "Begin download" << std::endl;
            break;
        case BINDSTATUS_DOWNLOADINGDATA:
        case BINDSTATUS_ENDDOWNLOADDATA:
        {
            auto percent = static_cast<int>(100.0 * static_cast<double>(ulProgress)
                / static_cast<double>(ulProgressMax));
            if (m_percentLast < percent) {
                LoadBar(percent, 100);
                m_percentLast = percent;
            }
            if (ulStatusCode == BINDSTATUS_ENDDOWNLOADDATA) {
                std::cout << std::endl << "End download" << std::endl;
            }
        }
        break;

        default:
        {
            std::cout << "Status code : " << ulStatusCode << std::endl;
        }
        }
        // The download can be cancelled by returning E_ABORT here
        // of from any other of the methods.
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE
        OnStopBinding(HRESULT /*hresult*/,
            LPCWSTR /*szError*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
        GetBindInfo(DWORD*    /*grfBINDF*/,
            BINDINFO* /*pbindinfo*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
        OnDataAvailable(DWORD      /*grfBSCF*/,
            DWORD      /*dwSize*/,
            FORMATETC* /*pformatetc*/,
            STGMEDIUM* /*pstgmed*/) override
    {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE
        OnObjectAvailable(REFIID    /*riid*/,
            IUnknown* /*punk*/) override
    {
        return E_NOTIMPL;
    }
};



int
main(int argc, char **argv)
{
    HRESULT hr;
    CallbackHandler callbackHandler;
    IBindStatusCallback* pBindStatusCallback = nullptr;

    callbackHandler.QueryInterface(IID_IBindStatusCallback, reinterpret_cast<void**>(&pBindStatusCallback));
    DeleteUrlCacheEntryA(argv[1]);
    std::cout << "Resource: " << argv[1] << std::endl;
    hr = URLDownloadToFileA(nullptr, argv[1], argv[2], 0, pBindStatusCallback);
    return hr == S_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
