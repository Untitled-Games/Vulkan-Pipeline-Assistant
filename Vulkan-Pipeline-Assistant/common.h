#ifndef COMMON_H
#define COMMON_H

#ifdef __clang__
#pragma clang diagnostic push
#if __has_warning("-Wzero-as-null-pointer-constant")
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wexit-time-destructors"
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif
#endif

#include <QString>

#define TEXDIR "../../Resources/Textures/"
#define CONFIGDIR "../../Resources/Configs/"
#define IMAGEDIR "../../Resources/Images/"
#define MESHDIR "../../Resources/Meshes/"

#define BYTE_CPTR(ptr) reinterpret_cast<const unsigned char*>(ptr)

// VPAError macros
#define VPA_OK VPAError(VPAErrorLevel::Ok, "")
#define VPA_WARN(msg) VPAError(VPAErrorLevel::Warning, msg)
#define VPA_CRITICAL(msg) VPAError(VPAErrorLevel::Critical, msg)
#define VPA_FATAL(msg) { QMessageBox::critical(nullptr, "VPA Fatal Error", msg);  QCoreApplication::quit(); } // Sources using this macro must include <QMessageBox> and <QCoreApplication>
#define VPA_PASS_ERROR(err) if (err.level != VPAErrorLevel::Ok) return err

// VPAError decoration for vulkan functions which return a VkResult
#define VKRESULT_MESSAGE(result, name) "VkResult for" + QString(name) + "error code" + QString::number(result)
#define VPA_VKCRITICAL(result, name, err) if (result != VK_SUCCESS) err = VPA_CRITICAL(VKRESULT_MESSAGE(result, name));
#define VPA_VKCRITICAL_CTOR(result, name, err) if (result != VK_SUCCESS) err = VPA_CRITICAL(VKRESULT_MESSAGE(result, name));
#define VPA_VKCRITICAL_PASS(result, name) if (result != VK_SUCCESS) { VPA_PASS_ERROR(VPA_CRITICAL(VKRESULT_MESSAGE(result, name))); }
#define VPA_VKCRITICAL_CTOR_PASS(result, name, err) if (result != VK_SUCCESS) { err = VPA_CRITICAL(VKRESULT_MESSAGE(result, name)); return; }

// This should be used if there is some unrecoverable invalid state caused by a vulkan command
#define VPA_VKFATAL(result, name) if (result != VK_SUCCESS) VPA_FATAL(VKRESULT_MESSAGE(result, name))

#define DESTROY_HANDLE(device, handle, func) if (handle != VK_NULL_HANDLE) { func(device, handle, nullptr); handle = VK_NULL_HANDLE; }

namespace vpa {
    enum class VPAErrorLevel {
        Ok, // No error
        Warning, // Error created by invalid config
        Critical // Error created by vulkan
    };

    struct VPAError {
        VPAErrorLevel level;
        QString message;
        VPAError(VPAErrorLevel lvl, QString msg) : level(lvl), message(msg) { }
    };
}

#endif // COMMON_H
