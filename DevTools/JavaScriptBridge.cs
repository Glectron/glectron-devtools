using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using System.Reflection;

namespace DevTools
{
    internal class JavaScriptBridge
    {
        private readonly HttpClient hc = new();

        public class InjectorState
        {
            public int ProcessId;
            public Injector.InjectStatus Status;
            public string? Title;
            public int DebuggingPort;
        }

        public InjectorState[] GetInjectorState()
        {
            return [.. Program.Injectors.Select((v) =>
            {
                return new InjectorState
                {
                    ProcessId = v.Key,
                    Status = v.Value.Status,
                    Title = v.Value.Title,
                    DebuggingPort = v.Value.DebuggingPort
                };
            })];
        }

        public async Task<string> PerformRequest(int processId, string path)
        {
            var injector = Program.Injectors[processId] ?? throw new ArgumentException("Invalid process ID.");
            if (injector.DebuggingPort == 0)
                throw new ArgumentException($"{processId} isn't injected.");
            var res = await hc.GetAsync("http://localhost:" + injector.DebuggingPort + path);
            return await res.Content.ReadAsStringAsync();
        }

        public void OpenDevTools(int processId, string id, string ws, string? title)
        {
            if (Program.MainWindow == null) throw new InvalidOperationException("Main window is not available.");
            var injector = Program.Injectors[processId] ?? throw new ArgumentException("Invalid process ID.");
            Program.MainWindow.OpenDevToolsWindow(injector, id, ws, title);
        }

        public void SetDevToolsTitle(int processId, string id, string title)
        {
            if (Program.MainWindow == null) throw new InvalidOperationException("Main window is not available.");
            var injector = Program.Injectors[processId] ?? throw new ArgumentException("Invalid process ID.");
            Program.MainWindow.SetDevToolsWindowTitle(injector, id, title);
        }

        public void SetSetting(string name, object value)
        {
            if (string.IsNullOrWhiteSpace(name))
                throw new ArgumentException("Setting name is required.", nameof(name));

            var settings = InjectorSettings.GlobalSettings;
            var type = typeof(InjectorSettings);

            // Try fields first (ignore case)
            var field = type.GetField(name, BindingFlags.Instance | BindingFlags.Public | BindingFlags.IgnoreCase);
            if (field != null)
            {
                var converted = ConvertToType(value, field.FieldType);
                field.SetValue(settings, converted);
                return;
            }

            // Try properties
            var prop = type.GetProperty(name, BindingFlags.Instance | BindingFlags.Public | BindingFlags.IgnoreCase);
            if (prop != null && prop.CanWrite)
            {
                var converted = ConvertToType(value, prop.PropertyType);
                prop.SetValue(settings, converted);
                return;
            }

            throw new ArgumentException($"Unknown setting: {name}", nameof(name));

            static object? ConvertToType(object? val, Type targetType)
            {
                if (val is null)
                {
                    if (targetType.IsValueType && Nullable.GetUnderlyingType(targetType) == null)
                        throw new InvalidCastException($"Cannot assign null to non-nullable type {targetType.Name}.");
                    return null;
                }

                var nonNullable = Nullable.GetUnderlyingType(targetType) ?? targetType;

                if (nonNullable.IsEnum)
                {
                    if (val is string s)
                        return Enum.Parse(nonNullable, s, ignoreCase: true);
                    var underlyingEnumType = Enum.GetUnderlyingType(nonNullable);
                    var convertedNumber = Convert.ChangeType(val, underlyingEnumType);
                    return Enum.ToObject(nonNullable, convertedNumber!);
                }

                if (nonNullable == typeof(Guid) && val is string gs)
                    return Guid.Parse(gs);

                return Convert.ChangeType(val, nonNullable)!;
            }
        }
    }
}
