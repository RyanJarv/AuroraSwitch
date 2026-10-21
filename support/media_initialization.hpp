#pragma once

// Ordered media setup shared by firmware and host failure tests.
namespace daisy_development
{
    enum class MediaInitialization { Ready, UsbFailed, FilesystemFailed, MountFailed };

    // Stop at the first failure: later services must not consume uninitialized
    // handles. The caller must refuse all media/control work unless Ready.
    template<class Usb, class Filesystem, class Mount>
    MediaInitialization InitializeMedia(Usb usb, Filesystem filesystem, Mount mount)
    {
        if(!usb()) return MediaInitialization::UsbFailed;
        if(!filesystem()) return MediaInitialization::FilesystemFailed;
        if(!mount()) return MediaInitialization::MountFailed;
        return MediaInitialization::Ready;
    }
}
