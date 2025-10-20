using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DevTools
{
    public sealed class InjectorSettings
    {
        public static InjectorSettings GlobalSettings = new();

        public bool DisableSandbox = false;
        public bool RegisterAssetAsSecured = false;

        public InjectorSettings()
        {
        }

        public InjectorSettings Clone()
        {
            return new InjectorSettings
            {
                DisableSandbox = DisableSandbox,
                RegisterAssetAsSecured = RegisterAssetAsSecured
            };
        }

        public InjectorSettingsNative ToNative()
        {
            return new InjectorSettingsNative
            {
                DisableSandbox = DisableSandbox,
                RegisterAssetAsSecured = RegisterAssetAsSecured
            };
        }

        public static InjectorSettings FromNative(InjectorSettingsNative native)
        {
            return new InjectorSettings
            {
                DisableSandbox = native.DisableSandbox,
                RegisterAssetAsSecured = native.RegisterAssetAsSecured
            };
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct InjectorSettingsNative
    {
        [MarshalAs(UnmanagedType.I1)]
        public bool DisableSandbox;

        [MarshalAs(UnmanagedType.I1)]
        public bool RegisterAssetAsSecured;
    }
}
