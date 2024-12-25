#include "xlog.h"

#include <assert.h>
#include <fmt/chrono.h>
#include <fmt/color.h>

#include <ulib/string.h>
#include <ulib/chrono.h>

namespace xlog
{
    Logger::System &Logger::System::Instance()
    {
        static Logger::System ls;
        return ls;
    }

    Logger::System::System() {}
    Logger::System::~System() {}

    void Logger::System::SetParent(Logger *pLogger)
    {
        // suka blyat
        assert(pLogger->GetSystem() == this);
        mLoggers.Add(pLogger);
    }

    void Logger::System::UnsetParent(Logger *pLogger)
    {
        // suka blyat
        assert(pLogger->GetSystem() == this);
        mLoggers.Remove(pLogger);

        for (auto &obj : mLoggers)
        {
            if (obj->mParent == pLogger)
                obj->mParent = pLogger->mParent;
        }
    }

    Logger::Logger(ulib::u8string_view name, Flags flags)
    {
        mSystem = &Logger::System::Instance();
        mParent = nullptr;
        mFlags = flags;
        mConsole = true;
        mSystem->SetParent(this);
    }

    Logger::Logger(ulib::u8string_view name, Logger *pLogger, Flags flags)
    {
        mParent = pLogger;
        mSystem = pLogger->mSystem;
        mFlags = flags;
        mConsole = true;

        std::lock_guard lock(mSystem->Sync());

        mSystem->SetParent(this);
    }

    Logger::~Logger()
    {
        std::lock_guard lock(mSystem->Sync());

        mSystem->UnsetParent(this);

        for (auto &obj : mFiles)
            delete obj;
    }

    FileOutput::FileOutput(const fs::path &path)
    {
        mPath = path;

        fs::path dir = mPath;
        dir.remove_filename();

        if (!dir.empty())
            fs::create_directories(dir);

        mFile = std::make_unique<futile::File>(futile::open(mPath, "a"));
    }

    FileOutput::~FileOutput() {}

    void Logger::AddFile(const fs::path &path)
    {
        std::lock_guard lock(mSystem->Sync());
        mFiles.Add(new FileOutput(path));
    }

    void Logger::AddPrefix(ulib::u8string_view pref)
    {
        std::lock_guard lock(mSystem->Sync());
        mPrefixes.Add(pref);
    }

    void Logger::AddHandler(ILoggerHandler *pHandler)
    {
        std::lock_guard lock(mSystem->Sync());
        mHandlers.Add(pHandler);
    }

    void Logger::Print(LogType type, ulib::u8string_view str)
    {
        std::lock_guard lock(mSystem->Sync());
        auto date = ulib::format(u8"[{:%Y-%m-%d %H:%M:%S}]", fmt::localtime(ulib::unix_time().count()));

        const char *ptype = nullptr;
        fmt::text_style style;

        mBuffer.Clear();
        if (!(mFlags & Flag_DisableTypePrefixes))
        {
            switch (type)
            {
            case LogType::Info:
                ptype = "[info]";             
                break;
            case LogType::Warn:
                ptype = "[warn]";
                style = fmt::fg(fmt::color::orange);
                break;
            case LogType::Critical:
                ptype = "[critical]";
                style = fmt::fg(fmt::color::red);
                break;
            }
        }

        CollectPrefixes(mBuffer);

        if (!(mFlags & Flag_IgnoreConsole))
        {
            fmt::print("{} ", date);
            if (ptype)
                fmt::print(style, "{} ", ptype);
            fmt::print("{}{}\n", mBuffer, str);
        }

        if (ptype)
            mBuffer = ulib::format(u8"{} {} {} {}\n", date, ptype, mBuffer, str);
        else
            mBuffer = ulib::format(u8"{} {} {}\n", date, mBuffer, str);

        PrintToFiles(type, mBuffer);
        CallOnMessage(type, mBuffer);
    }

    void Logger::SetFlag(Flags flag, bool enable)
    {
        mFlags = enable ? mFlags | flag : mFlags & ~flag;
    }

    void Logger::CollectPrefixes(ulib::u8string &out)
    {
        if (mFlags & Flag_InheritPrefixes)
            if (mParent)
                mParent->CollectPrefixes(out);

        for (auto &obj : mPrefixes)
        {
            out += obj;
            out += u8" ";
        }
    }

    void Logger::PrintToFiles(LogType type, ulib::u8string_view str)
    {
        for (auto &obj : mFiles)
        {
            obj->file().write(ulib::format("{}", str));
            obj->file().flush();
        }

        if (mFlags & Flag_InheritFiles)
        {
            if (mParent)
                mParent->PrintToFiles(type, str);
        }
    }

    void Logger::CallOnMessage(LogType type, ulib::u8string_view str)
    {
        for (auto &obj : mHandlers)
            obj->OnMessage(type, str);

        if (mFlags & Flag_InheritHandlers)
        {
            if (mParent)
                mParent->CallOnMessage(type, str);
        }
    }

    void Logger::CallOnException(LogType type, ulib::u8string_view f, ulib::u8string_view str)
    {
        for (auto &h : mHandlers)
            h->OnException(type, f, str);

        if (mFlags & Flag_InheritHandlers)
        {
            if (mParent)
                mParent->CallOnException(type, f, str);
        }
    }
} // namespace xlog