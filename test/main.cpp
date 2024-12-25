#include <xlog/xlog.h>

struct LoggerHandler : public xlog::ILoggerHandler
{
	void OnMessage(xlog::LogType type, ulib::u8string_view msg)
	{
		if (type == xlog::LogType::Critical)
		{
			fmt::print("Critical error! msg: {}", msg);
		}
	}

	void OnException(xlog::LogType type, ulib::u8string_view format, ulib::u8string_view what)
	{
		fmt::print("Exception: format: {} what: {}", format, what);
		abort();
	}
};

int test2();
int test1();

int main()
{
    test2();
    test1();

	LoggerHandler handler;

	xlog::Logger log(u8"manager");
	log.AddHandler(&handler);
	log.AddFile("manager-0.log");
	log.AddFile("manager-1.log");
	log.AddFile("manager-2.log");
	log.AddFile("manager-3.log");
	log.AddFile("manager-4.log");
	log.AddPrefix(u8"[Manager]");
	log.Info("Starting initialization");

	xlog::Logger ilog(u8"manager.initialization", &log, xlog::Flag_InheritAll);
	ilog.AddFile("manager.initialization.log");
	ilog.AddPrefix(u8"[Initialization]");
	ilog.Info("Environment initialized.");
	ilog.Info("Finished.");

	log.Info("Transform to EVIL entity...");
	log.AddPrefix(u8"[!EVIL!]");

	log.Info("I`m EVIL.");
	ilog.Info("I`m too.");

	ilog.Critical("EVIL EVERYONE");

	return 0;
}