#include "EngineCore/Logging/logger_service.h"
#include "EngineCore/Logging/logger.h"
#include "EngineCore/Pipeline/variant.h"
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

void LoggerService::LoggerRoutine(moodycamel::ConcurrentQueue<LogEvent>* queue)
{
    LogEvent event;
    SleepCounter counter;

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
            case Engine::Core::Pipeline::VariantType::Byte:
                printf("0x%02x", event.Payload.Parameter.Data.Byte);
                break;
            case Engine::Core::Pipeline::VariantType::Bool:
                printf("%s", event.Payload.Parameter.Data.Bool ? "TRUE" : "FALSE");
                break;
            case Engine::Core::Pipeline::VariantType::Int32:
                printf("%d", event.Payload.Parameter.Data.Int32);
                break;
            case Engine::Core::Pipeline::VariantType::Int64:
                printf("%ld", event.Payload.Parameter.Data.Int64);
                break;
            case Engine::Core::Pipeline::VariantType::Uint32:
                printf("%u", event.Payload.Parameter.Data.Uint32);
                break;
            case Engine::Core::Pipeline::VariantType::Uint64:
                printf("%lu", event.Payload.Parameter.Data.Uint64);
                break;
            case Engine::Core::Pipeline::VariantType::Float:
                printf("%.3f", event.Payload.Parameter.Data.Float);
                break;
            case Engine::Core::Pipeline::VariantType::Vec2:
                printf("<%.3f, %.3f>", event.Payload.Parameter.Data.Vec2.x, event.Payload.Parameter.Data.Vec2.y);
                break;
            case Engine::Core::Pipeline::VariantType::Vec3:
                printf("<%.3f, %.3f, %.3f>", event.Payload.Parameter.Data.Vec3.x, event.Payload.Parameter.Data.Vec3.y, event.Payload.Parameter.Data.Vec3.z);
                break;
            case Engine::Core::Pipeline::VariantType::Vec4:
                printf("<%.3f, %.3f, %.3f, %.3f>", 
                    event.Payload.Parameter.Data.Vec4.x,
                    event.Payload.Parameter.Data.Vec4.y,
                    event.Payload.Parameter.Data.Vec4.z,
                    event.Payload.Parameter.Data.Vec4.w);
                break;
            case Engine::Core::Pipeline::VariantType::Mat2:
                printf("{<%.3f, %.3f>, <%.3f, %.3f>}", 
                    event.Payload.Parameter.Data.Mat2[0].x,
                    event.Payload.Parameter.Data.Mat2[0].y,
                    event.Payload.Parameter.Data.Mat2[1].x,
                    event.Payload.Parameter.Data.Mat2[1].y);
                break;
            case Engine::Core::Pipeline::VariantType::Mat3:
                printf("{<%.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f>}", 
                    event.Payload.Parameter.Data.Mat3[0].x,
                    event.Payload.Parameter.Data.Mat3[0].y,
                    event.Payload.Parameter.Data.Mat3[0].z,
                    event.Payload.Parameter.Data.Mat3[1].x,
                    event.Payload.Parameter.Data.Mat3[1].y,
                    event.Payload.Parameter.Data.Mat3[1].z,
                    event.Payload.Parameter.Data.Mat3[2].x,
                    event.Payload.Parameter.Data.Mat3[2].y,
                    event.Payload.Parameter.Data.Mat3[2].z);
                break;
            case Engine::Core::Pipeline::VariantType::Mat4:
                printf("{<%.3f, %.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f, %.3f>, <%.3f, %.3f, %.3f, %.3f>}", 
                    event.Payload.Parameter.Data.Mat4[0].x,
                    event.Payload.Parameter.Data.Mat4[0].y,
                    event.Payload.Parameter.Data.Mat4[0].z,
                    event.Payload.Parameter.Data.Mat4[0].w,
                    event.Payload.Parameter.Data.Mat4[1].x,
                    event.Payload.Parameter.Data.Mat4[1].y,
                    event.Payload.Parameter.Data.Mat4[1].z,
                    event.Payload.Parameter.Data.Mat4[1].w,
                    event.Payload.Parameter.Data.Mat4[2].x,
                    event.Payload.Parameter.Data.Mat4[2].y,
                    event.Payload.Parameter.Data.Mat4[2].z,
                    event.Payload.Parameter.Data.Mat4[2].w,
                    event.Payload.Parameter.Data.Mat4[3].x,
                    event.Payload.Parameter.Data.Mat4[3].y,
                    event.Payload.Parameter.Data.Mat4[3].z,
                    event.Payload.Parameter.Data.Mat4[3].w);
                break;
            case Engine::Core::Pipeline::VariantType::Path:
                printf("%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                    event.Payload.Parameter.Data.Path[0],
                    event.Payload.Parameter.Data.Path[1],
                    event.Payload.Parameter.Data.Path[2],
                    event.Payload.Parameter.Data.Path[3],
                    event.Payload.Parameter.Data.Path[4],
                    event.Payload.Parameter.Data.Path[5],
                    event.Payload.Parameter.Data.Path[6],
                    event.Payload.Parameter.Data.Path[7],
                    event.Payload.Parameter.Data.Path[8],
                    event.Payload.Parameter.Data.Path[9],
                    event.Payload.Parameter.Data.Path[10],
                    event.Payload.Parameter.Data.Path[11],
                    event.Payload.Parameter.Data.Path[12],
                    event.Payload.Parameter.Data.Path[13],
                    event.Payload.Parameter.Data.Path[14],
                    event.Payload.Parameter.Data.Path[15]);
                break;
            case Engine::Core::Pipeline::VariantType::Invalid:
                printf("<N/A>");    
                break;
            }
        }

        printf("\n");
    }
}

LoggerService::LoggerService(Configuration::ConfigurationProvider configs) 
    : m_Queue(configs.LoggerBufferSize), m_Thread(LoggerRoutine, &m_Queue), m_MinLevel(configs.MinimumLogLevel)
{
    m_Sequencer.store(0);
}

LoggerService::~LoggerService()
{
    LogEvent terminator;
    terminator.Type = LogEventType::Terminator;
    
    // wait until the queue accepts this terminator event
    while (!m_Queue.try_enqueue(terminator)) {}

    m_Thread.join();
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