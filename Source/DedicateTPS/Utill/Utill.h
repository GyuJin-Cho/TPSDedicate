// 메시지박스
#pragma once


#define MSG_IMPL(Category, Format, ...) \
	FMessageDialog::Open(Category, EAppMsgType::Ok, FText::FromString(FString::Printf(Format, ##__VA_ARGS__)))

#define INFO_MSG(Format, ...)    MSG_IMPL(EAppMsgCategory::Info,    Format, ##__VA_ARGS__)
#define WARNING_MSG(Format, ...) MSG_IMPL(EAppMsgCategory::Warning, Format, ##__VA_ARGS__)
#define ERROR_MSG(Format, ...)   MSG_IMPL(EAppMsgCategory::Error,   Format, ##__VA_ARGS__)

// 로그
#define INFO_LOG(Format, ...)    UE_LOG(LogTemp, Log,     Format, ##__VA_ARGS__)
#define WARNING_LOG(Format, ...) UE_LOG(LogTemp, Warning, Format, ##__VA_ARGS__)
#define ERROR_LOG(Format, ...)   UE_LOG(LogTemp, Error,   Format, ##__VA_ARGS__)