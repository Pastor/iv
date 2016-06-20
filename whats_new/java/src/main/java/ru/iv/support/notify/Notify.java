package ru.iv.support.notify;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import ru.iv.support.Event;

public abstract class Notify {
    private static final Gson GSON = new GsonBuilder().setPrettyPrinting().create();

    public static Notify of(Event event) {
        if (event.type == Event.Type.PACKETS) {
            return new PacketsNotify(event);
        }
        return new StatusNotify(event);
    }

    public String toString() {
        return GSON.toJson(this, getClass());
    }
}
