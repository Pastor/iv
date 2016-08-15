namespace SupController.Model
{
    public class Event
    {
        public EventType Type;
    }

    public sealed class ProcessEvent
    {
        public long MsComplete;
        public long MsFullTime;
    }

    public enum EventType
    {
        DEVICE_CONNECTED,
        DEVICE_DISCONNECTED,
        DEVICE_PACKET,

        QUESTION_START,
        QUESTION_PROCESS,
        QUESTION_COMPLETE,
        QUESTION_CANCEL
    }
}
