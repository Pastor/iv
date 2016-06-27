package ru.iv.support.notify;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import ru.iv.support.Event;

@SuppressWarnings("unused")
public abstract class Notify {
    private static final Gson GSON = new GsonBuilder().setPrettyPrinting().create();

    @SuppressWarnings("FieldCanBeLocal")
    public final Type type;

    Notify(Type type) {
        this.type = type;
    }

    public static Notify of(Event event) {
        if (event.type == Event.Type.PACKETS) {
            return new PacketsNotify(event);
        }
        return new StatusNotify(event);
    }

    public static Notify of(long msComplete, long msFullTime) {
        return new ProcessNotify(msComplete, msFullTime);
    }

    public static Notify of(Type type) {
        return new SimpleNotify(type);
    }

    public String toString() {
        return GSON.toJson(this, getClass());
    }


    public enum Type {
        DEVICE_CONNECTED,
        DEVICE_DISCONNECTED,
        DEVICE_RECEIVE,
        DEVICE_SENT,
        QUESTION_START,
        QUESTION_COMPLETE,
        QUESTION_CANCEL,
        QUESTION_PROCESS
    }
}
