package ru.iv.support.dll;

import ru.iv.support.Device;
import ru.iv.support.Firmware;
import ru.iv.support.Packet;

import java.io.File;

final class Library {
    private static final boolean LOADED;
    private static final String KEY_LIBRARY_PATH = "java.library.path";
    private static final String USER_DIRECTORY = System.getProperty("user.dir");

    static {
        final StringBuilder libraryPath = new StringBuilder(System.getProperty(KEY_LIBRARY_PATH));
        final File up = new File(USER_DIRECTORY).getParentFile();
        final File build = new File(up, "native" + File.separator + ".build" + File.separator + "bin");
        registerPath(libraryPath, build.getAbsolutePath());
        registerPath(libraryPath, USER_DIRECTORY, "lib");
        registerPath(libraryPath, USER_DIRECTORY, "3rdParty");
        System.setProperty(KEY_LIBRARY_PATH, libraryPath.toString());
        LOADED = loadLibrary();
        Runtime.getRuntime().addShutdownHook(new Thread(Library::destroy));
    }

    private static boolean loadLibrary() {
        try {
            System.loadLibrary("support");
            return init();
        } catch (Exception e) {
            return false;
        }
    }

    static native void register(Callback callback);

    static native void send(Device device, String command, boolean calcHash);

    static native boolean hasDevice(Device device);

    private static native boolean init();

    private static native boolean destroy();

    private static void registerPath(final StringBuilder libraryPath, String... paths) {
        int i = 0;

        libraryPath.append(File.pathSeparator);
        for (String path : paths) {
            if (i > 0) {
                libraryPath.append(File.separator);
            }
            ++i;
            libraryPath.append(path);
        }
    }

    public static void main(String[] args) throws InterruptedException {
        if (LOADED) {
            register(new Callback() {
                @Override
                public void connected(Device device, Firmware firmware) {
                    System.out.println("Connected: " + device + " -> " + firmware);
                }

                @Override
                public void disconnected(Device device, Firmware firmware) {
                    System.out.println("Disconnected: " + device + " -> " + firmware);
                }

                @Override
                public void handle(Device device, Firmware firmware, Packet[] packets, int count) {
                    for (int i = 0; i < count; i++) {
                        System.out.println(packets[i]);
                    }
                }
            });
            System.out.println("Loaded");
            Thread.currentThread().join(5000);
            send(Device.HID, "Q2D\r", true);
        } else {
            System.out.println("Not loaded");
        }
    }
}
