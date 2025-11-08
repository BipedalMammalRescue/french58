#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Runtime/crash_dump.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_thread.h"
#include "SDL3/SDL_timer.h"

#include <cmath>
#include <stdio.h>

using namespace Engine::Core::Logging;

static constexpr int SpinCycleCount = 1000;
static constexpr int SleepSteps = 10;

class SleepCounter
{
private:
    unsigned int m_State;
    
public:
    void Wait()
    {
        static const int MaxSleepLength = std::pow(2, SleepSteps);

        // spin for 100 cycles
        if (m_State <= SpinCycleCount)
        {
            m_State++;
            return;
        }

        // sleep with a exponential number (10 stages)
        if (m_State - SpinCycleCount <= SleepSteps)
        {
            SDL_Delay(std::pow(2, m_State - SpinCycleCount));
            m_State ++;
            return;
        }

        // sleep for about a full second when it's already too far
        SDL_Delay(MaxSleepLength);
        return;
    }


    void Reset()
    {
        m_State = 0;
    }
};

int LoggerService::LoggerRoutine(void* state)
{
    LogEvent event;
    SleepCounter counter;

    auto queue = (moodycamel::ConcurrentQueue<LogEvent>*)state;

    while (true)
    {
        // exponential back off
        while (!queue->try_dequeue(event))
        {
            counter.Wait();
        }
        counter.Reset();

        if (event.Type == LogEventType::Terminator)
            break;

        printf("[SEQ:%d]", event.Sequence);

        if (event.Type == LogEventType::Header)
        {
            // time
            size_t hours = event.Payload.Header.Timestamp / 1000 / 60 / 60;
            size_t minutes = (event.Payload.Header.Timestamp / 1000 / 60) % 60;
            size_t seconds = (event.Payload.Header.Timestamp / 1000) % 60;
            size_t ms = event.Payload.Header.Timestamp % 1000;

            printf("[%02lu:%02lu:%02lu.%03lu ", hours, minutes, seconds, ms);

            // level
            switch (event.Payload.Header.Level)
            {
            case LogLevel::Verbose:
                printf("VER]");
                break;
            case LogLevel::Debug:
                printf("DBG]");
                break;
            case LogLevel::Information:
                printf("INF]");
                break;
            case LogLevel::Warning:
                printf("WRN]");
                break;
            case LogLevel::Error:
                printf("ERR]");
                break;
            case LogLevel::Fatal:
                printf("FTL]");
                break;
            default:
                continue;
            }

            // thread
            printf("[TID:%lu]", event.Payload.Header.ThreadId);

            // channels
            for (size_t i = 0; i < event.Payload.Header.ChannelCount; i++)
            {
                printf("[%s]", event.Payload.Header.Channels[i]);
            }

            // message
            printf("%s", event.Payload.Header.Message);
        }
        else if (event.Type == LogEventType::Parameter)
        {
            switch (event.Payload.Parameter.Type)
            {
            case LogParameterType::FixedSize:
                switch (event.Payload.Parameter.Data.Fixed.Type)
                {
                case Engine::Core::Pipeline::VariantType::Byte:
                    printf("0x%02x", event.Payload.Parameter.Data.Fixed.Data.Byte);
                    break;
                case Engine::Core::Pipeline::VariantType::Bool:
                    printf("%s", event.Payload.Parameter.Data.Fixed.Data.Bool ? "TRUE" : "FALSE");
                    break;
                case Engine::Core::Pipeline::VariantType::Int32:
                    printf("%d", event.Payload.Parameter.Data.Fixed.Data.Int32);
                    break;
                case Engine::Core::Pipeline::VariantType::Uint32:
                    printf("%u", event.Payload.Parameter.Data.Fixed.Data.Uint32);
                    break;
                case Engine::Core::Pipeline::VariantType::Float:
                    printf("%.3f", event.Payload.Parameter.Data.Fixed.Data.Float);
                    break;
                case Engine::Core::Pipeline::VariantType::Vec2:
                    printf("<%.3f, %.3f>", event.Payload.Parameter.Data.Fixed.Data.Vec2.x, event.Payload.Parameter.Data.Fixed.Data.Vec2.y);
                    break;
                case Engine::Core::Pipeline::VariantType::Vec3:
                    printf("<%.3f, %.3f, %.3f>", event.Payload.Parameter.Data.Fixed.Data.Vec3.x, event.Payload.Parameter.Data.Fixed.Data.Vec3.y, event.Payload.Parameter.Data.Fixed.Data.Vec3.z);
                    break;
                case Engine::Core::Pipeline::VariantType::Vec4:
                    printf("<%.3f, %.3f, %.3f, %.3f>", 
                        event.Payload.Parameter.Data.Fixed.Data.Vec4.x,
                        event.Payload.Parameter.Data.Fixed.Data.Vec4.y,
                        event.Payload.Parameter.Data.Fixed.Data.Vec4.z,
                        event.Payload.Parameter.Data.Fixed.Data.Vec4.w);
                    break;
                case Engine::Core::Pipeline::VariantType::Mat2:
                    printf("{<%.3f, %.3f>, <%.3f, %.3f>}", 
                        event.Payload.Parameter.Data.Fixed.Data.Mat2[0].x,
                        event.Payload.Parameter.Data.Fixed.Data.Mat2[0].y,
                        event.Payload.Parameter.Data.Fixed.Data.Mat2[1].x,
                        event.Payload.Parameter.Data.Fixed.Data.Mat2[1].y);
                    break;
                case Engine::Core::Pipeline::VariantType::Mat3:
                    printf("{<%.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f>}", 
                        event.Payload.Parameter.Data.Fixed.Data.Mat3[0].x,
                        event.Payload.Parameter.Data.Fixed.Data.Mat3[0].y,
                        event.Payload.Parameter.Data.Fixed.Data.Mat3[0].z,
                        event.Payload.Parameter.Data.Fixed.Data.Mat3[1].x,
                        event.Payload.Parameter.Data.Fixed.Data.Mat3[1].y,
                        event.Payload.Parameter.Data.Fixed.Data.Mat3[1].z,
                        event.Payload.Parameter.Data.Fixed.Data.Mat3[2].x,
                        event.Payload.Parameter.Data.Fixed.Data.Mat3[2].y,
                        event.Payload.Parameter.Data.Fixed.Data.Mat3[2].z);
                    break;
                case Engine::Core::Pipeline::VariantType::Mat4:
                    printf("{<%.3f, %.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f, %.3f>}", 
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[0].x,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[0].y,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[0].z,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[0].w,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[1].x,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[1].y,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[1].z,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[1].w,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[2].x,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[2].y,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[2].z,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[2].w,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[3].x,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[3].y,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[3].z,
                        event.Payload.Parameter.Data.Fixed.Data.Mat4[3].w);
                    break;
                case Engine::Core::Pipeline::VariantType::Path:
                    printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[0],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[1],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[2],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[3],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[4],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[5],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[6],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[7],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[8],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[9],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[10],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[11],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[12],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[13],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[14],
                        event.Payload.Parameter.Data.Fixed.Data.Path.Hash[15]);
                    break;
                case Engine::Core::Pipeline::VariantType::Invalid:
                    printf("<N/A>");    
                    break;
                }
                break;
            case LogParameterType::String:
                if (event.Payload.Parameter.Data.String == nullptr)
                {
                    printf("(null)");
                }
                else 
                {
                    printf("%s", event.Payload.Parameter.Data.String);
                }
                break;
            }
        }

        printf("\n");
    }

    return 0;
}

LoggerService::LoggerService(Configuration::ConfigurationProvider configs) 
    : m_Queue(configs.LoggerBufferSize), m_Thread(nullptr), m_MinLevel(configs.MinimumLogLevel)
{
    m_Sequencer.store(0);
}

Engine::Core::Runtime::CallbackResult LoggerService::StartLogger()
{
    m_Thread = SDL_CreateThread(LoggerRoutine, "Logger Thread", &m_Queue);
    if (m_Thread == nullptr)
        return Runtime::Crash(__FILE__, __LINE__, std::string("Failed to create logger thread, details: ") + SDL_GetError());
    return Runtime::CallbackSuccess();
}

LoggerService::~LoggerService()
{
    LogEvent terminator;
    terminator.Type = LogEventType::Terminator;
    
    // wait until the queue accepts this terminator event
    while (!m_Queue.try_enqueue(terminator)) {}

    SDL_WaitThread(m_Thread, nullptr);
}

bool LoggerService::Write(const LogEvent& event)
{
    return m_Queue.try_enqueue(event);
}

bool LoggerService::Write(const LogEvent&& event)
{
    return m_Queue.try_enqueue(event);
}

int LoggerService::ReserveSequence(int segmentCount)
{
    return m_Sequencer.fetch_add(segmentCount);
}

Logger LoggerService::CreateLogger(const char** channels, size_t channelCount)
{
    return Logger {
        channels,
        channelCount,
        this
    };
}