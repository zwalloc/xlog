#pragma once

#include <ulib/list.h>
#include <ulib/string.h>
#include <ulib/format.h>

#include <futile/futile.h>

#include <stdio.h>
#include <string>
#include <mutex>
#include <exception>
#include <filesystem>

/*

	xlog library
	that is log library with multithreading, prefixes, more files output, console output, behavior inherit supporting

*/

namespace xlog
{
    namespace fs = std::filesystem;

	enum Flag : uint
	{
		Flag_None = 0,

		Flag_InheritPrefixes = 1 << 0,
		Flag_InheritFiles = 1 << 1,
		Flag_InheritHandlers = 1 << 2,

		Flag_InheritAll = 1 | 2 | 4,

		Flag_IgnoreConsole = 1 << 3,
		Flag_DisableTypePrefixes = 1 << 4,
	};

	typedef uint Flags;

	class FileOutput
	{
	public:
		FileOutput(const fs::path& path);
		~FileOutput();

		futile::File& file() { return *mFile.get(); }
		const fs::path& path() { return mPath; }
		// void SetFile(FILE* pFile) { mFile.reset(pFile); }
		void Open();

	private:
        std::unique_ptr<futile::File> mFile;
		fs::path mPath;
	};

	enum class LogType
	{
		Info,
		Warn,
		Critical,
	};

	struct ILoggerHandler
	{
		virtual void OnMessage(LogType type, ulib::u8string_view msg) = 0;
		virtual void OnException(LogType type, ulib::u8string_view format, ulib::u8string_view what) = 0;
	};

	class Logger
	{
	public:
		class System
		{
		public:
			System();
			~System();

			static System& Instance();
			inline const ulib::List<Logger*>& GetLoggers() { return mLoggers; }
			inline std::mutex& Sync() { return mSync; }

			void SetParent(Logger* pLogger);
			void UnsetParent(Logger* pLogger);

		private:		

			ulib::List<Logger*> mLoggers;
			std::mutex mSync;
		};

		Logger(ulib::u8string_view name, Flags flags = Flag_None);
		Logger(ulib::u8string_view name, Logger* pLogger, Flags flags = Flag_None);
		~Logger();

		System* GetSystem() { return mSystem; }

		inline void SetConsole(bool v) { mConsole = v; }
		inline bool IsConsole() { return mConsole; }

		void AddFile(const fs::path& path);
		void AddPrefix(ulib::u8string_view pref);
		void AddHandler(ILoggerHandler* pHandler);

		void Print(LogType type, ulib::u8string_view text);
        void SetFlag(Flags flag, bool enable = true);
        bool GetFlag(Flags flag) { return mFlags & flag; }

		template<class F, class... Args>
		void Type(LogType type, F f, Args&&... args)
		{
			try
			{
				Print(type, ulib::format<ulib::Utf8>(f, args...));
			}
			catch (const std::exception& ex)
			{
				for (auto obj : mHandlers)
				{
					obj->OnException(type, ulib::u8(f), ulib::u8(ex.what()));
				}
			}
		}

		template<class F, class... Args>
		void Info(F f, Args&&... args)
		{
			Type(LogType::Info, f, args...);
		}

		template<class F, class... Args>
		void Warn(F f, Args&&... args)
		{
			Type(LogType::Warn, f, args...);
		}

		template<class F, class... Args>
		void Critical(F f, Args&&... args)
		{
			Type(LogType::Critical, f, args...);
		}

		
	private:

		void PrintToFiles(LogType type, ulib::u8string_view str);
		void CallOnMessage(LogType type, ulib::u8string_view str);
		void CallOnException(LogType type, ulib::u8string_view f, ulib::u8string_view str);
		void CollectPrefixes(ulib::u8string& out);

		Logger* mParent;
		System* mSystem;

		ulib::u8string mName;
		bool mConsole;

		ulib::List<ulib::u8string> mPrefixes;
		ulib::List<FileOutput*> mFiles;
		ulib::List<ILoggerHandler*> mHandlers;

		Flags mFlags;

		ulib::u8string mPrefsBuf;
		ulib::u8string mBuffer;
	};
}