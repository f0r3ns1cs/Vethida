#include "Services.h"

#include <QStringLiteral>

#include <vector>

#include <windows.h>
#include <winsvc.h>

namespace vth::win::services
{
    namespace
    {
        State mapState(DWORD scmState) {
            switch (scmState) {
            case SERVICE_STOPPED:          return State::Stopped;
            case SERVICE_START_PENDING:    return State::StartPending;
            case SERVICE_STOP_PENDING:     return State::StopPending;
            case SERVICE_RUNNING:          return State::Running;
            case SERVICE_CONTINUE_PENDING: return State::ContinuePending;
            case SERVICE_PAUSE_PENDING:    return State::PausePending;
            case SERVICE_PAUSED:           return State::Paused;
            }
            return State::Unknown;
        }

        DWORD scmStartType(StartType t) {
            switch (t) {
            case StartType::Auto:
            case StartType::AutoDelayed: return SERVICE_AUTO_START;
            case StartType::Manual:      return SERVICE_DEMAND_START;
            case StartType::Disabled:    return SERVICE_DISABLED;
            default:                     return SERVICE_NO_CHANGE;
            }
        }

    }

    QString stateLabel(State s) {
        switch (s) {
        case State::Running:          return QStringLiteral("Running");
        case State::Stopped:          return QStringLiteral("Stopped");
        case State::StartPending:     return QStringLiteral("Starting");
        case State::StopPending:      return QStringLiteral("Stopping");
        case State::ContinuePending:  return QStringLiteral("Continuing");
        case State::PausePending:     return QStringLiteral("Pausing");
        case State::Paused:           return QStringLiteral("Paused");
        case State::Unknown:          break;
        }
        return QStringLiteral("Unknown");
    }

    QString startTypeLabel(StartType t) {
        switch (t) {
        case StartType::Auto:        return QStringLiteral("Automatic");
        case StartType::AutoDelayed: return QStringLiteral("Automatic (Delayed)");
        case StartType::Manual:      return QStringLiteral("Manual");
        case StartType::Disabled:    return QStringLiteral("Disabled");
        case StartType::Boot:        return QStringLiteral("Boot");
        case StartType::System:      return QStringLiteral("System");
        case StartType::Unknown:     break;
        }
        return QStringLiteral("Unknown");
    }

    QString setStartTypeResultLabel(SetStartTypeResult r) {
        switch (r) {
        case SetStartTypeResult::Ok:
            return QStringLiteral("Change applied.");
        case SetStartTypeResult::AccessDenied:
            return QStringLiteral(
                "Windows protects this service. Security software and some "
                "core Windows services refuse start-type changes.");
        case SetStartTypeResult::NotFound:
            return QStringLiteral("Service not found on this system.");
        case SetStartTypeResult::InvalidStartType:
            return QStringLiteral(
                "Boot / System start types are reserved for drivers; "
                "use Automatic, Manual, or Disabled.");
        case SetStartTypeResult::OtherError:
            return QStringLiteral("Windows could not change this service.");
        }
        return {};
    }

    QVector<ServiceInfo> enumerate() {
        QVector<ServiceInfo> out;

        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr,
            SC_MANAGER_ENUMERATE_SERVICE);
        if (!scm) return out;

        DWORD bytesNeeded = 0;
        DWORD servicesCount = 0;
        DWORD resumeHandle = 0;

        EnumServicesStatusExW(
            scm, SC_ENUM_PROCESS_INFO,
            SERVICE_WIN32, SERVICE_STATE_ALL,
            nullptr, 0,
            &bytesNeeded, &servicesCount, &resumeHandle, nullptr);

        if (GetLastError() != ERROR_MORE_DATA && bytesNeeded == 0) {
            CloseServiceHandle(scm);
            return out;
        }

        std::vector<BYTE> buffer(bytesNeeded);
        if (!EnumServicesStatusExW(
            scm, SC_ENUM_PROCESS_INFO,
            SERVICE_WIN32, SERVICE_STATE_ALL,
            buffer.data(), static_cast<DWORD>(buffer.size()),
            &bytesNeeded, &servicesCount, &resumeHandle, nullptr)) {
            CloseServiceHandle(scm);
            return out;
        }

        auto* records = reinterpret_cast<ENUM_SERVICE_STATUS_PROCESSW*>(buffer.data());
        out.reserve(static_cast<int>(servicesCount));
        for (DWORD i = 0; i < servicesCount; ++i) {
            ServiceInfo info;
            info.name = QString::fromWCharArray(records[i].lpServiceName);
            info.displayName = QString::fromWCharArray(records[i].lpDisplayName);
            info.state = mapState(records[i].ServiceStatusProcess.dwCurrentState);
            out.append(std::move(info));
        }

        CloseServiceHandle(scm);
        return out;
    }

    State getState(const QString& serviceName) {
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scm) return State::Unknown;

        const std::wstring nameW = serviceName.toStdWString();
        SC_HANDLE svc = OpenServiceW(scm, nameW.c_str(), SERVICE_QUERY_STATUS);
        if (!svc) {
            CloseServiceHandle(scm);
            return State::Unknown;
        }

        SERVICE_STATUS status{};
        const BOOL ok = QueryServiceStatus(svc, &status);

        CloseServiceHandle(svc);
        CloseServiceHandle(scm);

        return ok ? mapState(status.dwCurrentState) : State::Unknown;
    }

    StartType getStartType(const QString& serviceName) {
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scm) return StartType::Unknown;

        const std::wstring nameW = serviceName.toStdWString();
        SC_HANDLE svc = OpenServiceW(scm, nameW.c_str(), SERVICE_QUERY_CONFIG);
        if (!svc) {
            CloseServiceHandle(scm);
            return StartType::Unknown;
        }

        DWORD bytesNeeded = 0;
        QueryServiceConfigW(svc, nullptr, 0, &bytesNeeded);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytesNeeded == 0) {
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return StartType::Unknown;
        }

        std::vector<BYTE> buffer(bytesNeeded);
        auto* config = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buffer.data());
        if (!QueryServiceConfigW(svc, config, bytesNeeded, &bytesNeeded)) {
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return StartType::Unknown;
        }

        StartType result = StartType::Unknown;
        switch (config->dwStartType) {
        case SERVICE_BOOT_START:    result = StartType::Boot;     break;
        case SERVICE_SYSTEM_START:  result = StartType::System;   break;
        case SERVICE_DEMAND_START:  result = StartType::Manual;   break;
        case SERVICE_DISABLED:      result = StartType::Disabled; break;
        case SERVICE_AUTO_START: {
            // Auto vs Auto-Delayed needs a separate QueryServiceConfig2.
            SERVICE_DELAYED_AUTO_START_INFO delayed{};
            DWORD needed2 = 0;
            QueryServiceConfig2W(svc, SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
                reinterpret_cast<LPBYTE>(&delayed),
                sizeof(delayed), &needed2);
            result = delayed.fDelayedAutostart ? StartType::AutoDelayed
                : StartType::Auto;
            break;
        }
        }

        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return result;
    }

    SetStartTypeResult setStartType(const QString& serviceName, StartType t) {
        if (t == StartType::Unknown ||
            t == StartType::Boot ||
            t == StartType::System) {
            return SetStartTypeResult::InvalidStartType;
        }

        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scm) {
            const DWORD err = GetLastError();
            return err == ERROR_ACCESS_DENIED ? SetStartTypeResult::AccessDenied
                : SetStartTypeResult::OtherError;
        }

        const std::wstring nameW = serviceName.toStdWString();
        SC_HANDLE svc = OpenServiceW(scm, nameW.c_str(), SERVICE_CHANGE_CONFIG);
        if (!svc) {
            const DWORD err = GetLastError();
            CloseServiceHandle(scm);
            if (err == ERROR_ACCESS_DENIED)         return SetStartTypeResult::AccessDenied;
            if (err == ERROR_SERVICE_DOES_NOT_EXIST) return SetStartTypeResult::NotFound;
            return SetStartTypeResult::OtherError;
        }

        SetStartTypeResult result = SetStartTypeResult::Ok;
        if (!ChangeServiceConfigW(
            svc,
            SERVICE_NO_CHANGE,
            scmStartType(t),
            SERVICE_NO_CHANGE,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr)) {
            const DWORD err = GetLastError();
            result = err == ERROR_ACCESS_DENIED ? SetStartTypeResult::AccessDenied
                : SetStartTypeResult::OtherError;
        }
        else if (t == StartType::Auto || t == StartType::AutoDelayed) {
            // A failure here is non-fatal: the start type has already changed.
            SERVICE_DELAYED_AUTO_START_INFO delayed{};
            delayed.fDelayedAutostart = (t == StartType::AutoDelayed) ? TRUE : FALSE;
            ChangeServiceConfig2W(svc,
                SERVICE_CONFIG_DELAYED_AUTO_START_INFO, &delayed);
        }

        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return result;
    }
}

namespace vth::win::services
{
    QStringList getDependencies(const QString& serviceName) {
        QStringList out;
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scm) return out;
        const std::wstring nameW = serviceName.toStdWString();
        SC_HANDLE svc = OpenServiceW(scm, nameW.c_str(), SERVICE_QUERY_CONFIG);
        if (!svc) {
            CloseServiceHandle(scm);
            return out;
        }
        DWORD bytesNeeded = 0;
        QueryServiceConfigW(svc, nullptr, 0, &bytesNeeded);
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytesNeeded == 0) {
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return out;
        }
        std::vector<BYTE> buffer(bytesNeeded);
        auto* config = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(buffer.data());
        if (!QueryServiceConfigW(svc, config, bytesNeeded, &bytesNeeded)) {
            CloseServiceHandle(svc);
            CloseServiceHandle(scm);
            return out;
        }
        if (config->lpDependencies) {
            const wchar_t* p = config->lpDependencies;
            while (*p) {
                const QString dep = QString::fromWCharArray(p);
                if (!dep.isEmpty() && !dep.startsWith(QLatin1Char('+'))) {
                    out.append(dep);
                }
                p += dep.size() + 1;
            }
        }
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return out;
    }
    QStringList getDependents(const QString& serviceName) {
        QStringList out;
        SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scm) return out;
        const std::wstring nameW = serviceName.toStdWString();
        SC_HANDLE svc = OpenServiceW(scm, nameW.c_str(),
            SERVICE_ENUMERATE_DEPENDENTS);
        if (!svc) {
            CloseServiceHandle(scm);
            return out;
        }
        DWORD bytesNeeded = 0;
        DWORD count = 0;
        EnumDependentServicesW(svc, SERVICE_STATE_ALL,
            nullptr, 0, &bytesNeeded, &count);
        if (GetLastError() == ERROR_MORE_DATA && bytesNeeded > 0) {
            std::vector<BYTE> buf(bytesNeeded);
            auto* arr = reinterpret_cast<ENUM_SERVICE_STATUSW*>(buf.data());
            if (EnumDependentServicesW(svc, SERVICE_STATE_ALL,
                arr, bytesNeeded, &bytesNeeded, &count)) {
                out.reserve(static_cast<int>(count));
                for (DWORD i = 0; i < count; ++i) {
                    if (arr[i].lpServiceName) {
                        out.append(QString::fromWCharArray(arr[i].lpServiceName));
                    }
                }
            }
        }
        CloseServiceHandle(svc);
        CloseServiceHandle(scm);
        return out;
    }
}

