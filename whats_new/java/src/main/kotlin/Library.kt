import java.util.concurrent.BlockingQueue
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit

data class Packet(val group: Int, val index: Int, val battery: Int, val timeout: Int, val enter: String) {
}

enum class EventType {
    CONNECTED, DISCONNECTED, RECEIVED
}

data class Event(
        val device: Device,
        val firmware: Firmware,
        val type: EventType,
        val packets: Array<Packet> = emptyArray()) {
}

enum class Device {
    HID, FTDI
}

enum class Firmware {
    FW42, FW60
}

interface Callback {
    fun connected(device: Device, firmware: Firmware)
    fun disconnected(device: Device, firmware: Firmware)
    fun handle(device: Device, firmware: Firmware, packets: Array<Packet>, count: Int)
}

open class SystemLibrary private constructor() {
    private val kLibraryPath = "java.library.path";

    init {
        if (loadLibrary() && init()) {
            register(object : Callback {
                override fun connected(device: Device, firmware: Firmware) {
                    events.add(Event(device, firmware, EventType.CONNECTED))
                }

                override fun disconnected(device: Device, firmware: Firmware) {
                    events.add(Event(device, firmware, EventType.DISCONNECTED))
                }

                override fun handle(device: Device, firmware: Firmware, packets: Array<Packet>, count: Int) {
                    events.add(Event(device, firmware, EventType.RECEIVED, packets.sliceArray(IntRange(0, count))));
                }
            })
        }
    }

    open val events: BlockingQueue<Event> = LinkedBlockingQueue(1000)

    open fun device(device: Device): Boolean {
        return hasDevice(device);
    }

    open fun sendCommand(device: Device, command: String): Unit {
        return send(device, command, true);
    }

    private external fun register(cb: Callback);

    private external fun send(device: Device, command: String, calcHash: Boolean)

    private external fun hasDevice(device: Device): Boolean

    private external fun init(): Boolean

    private external fun destroy(): Boolean

    private fun loadLibrary(): Boolean {
        System.loadLibrary("support")
        return true
    }

    private object Holder {
        val INSTANCE = SystemLibrary()
    }

    companion object {
        val default: SystemLibrary by lazy { Holder.INSTANCE }
    }
}

fun main(args: Array<String>) {
    val events: BlockingQueue<Event> = SystemLibrary.default.events;
    val runnable = Runnable {
        while (true) {
            val event = try {
                events.poll(100, TimeUnit.MILLISECONDS)
            } catch(e: InterruptedException) {
                null
            }
            if (event != null) {
                System.out.println(event)
            }
        }
    };
    val thread = Thread(runnable)
    thread.start()
    thread.join()
}
