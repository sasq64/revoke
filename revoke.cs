#if (UNITY_EDITOR_WIN || UNITY_STANDALONE_WIN)
#define WIN
#endif

using System;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.Linq.Expressions;

// A native reference to a managed object. Will not be garbage collected
// Until released
using NativeRef = System.IntPtr;

namespace revoke
{
    public class Expose : Attribute
    {
    };

    public static class Revoke
    {



#if WIN
        // On windows, we link the needed function at runtime

        // Copy a native string to another
        public delegate void StringCopy(IntPtr dest, IntPtr src);
        public delegate void revoke_callbacks(int count, IntPtr[] callbacks, IntPtr[] names);
        public static IntPtr revokeLibPtr;

        public static void Invoke<T>(params object[] pars)
        {
            IntPtr funcPtr = GetProcAddress(revokeLibPtr, typeof(T).Name);
            var func = Marshal.GetDelegateForFunctionPointer(funcPtr, typeof(T));
            func.DynamicInvoke(pars);
        }

        [DllImport("kernel32", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool FreeLibrary(IntPtr hModule);

        [DllImport("kernel32", SetLastError = true, CharSet = CharSet.Unicode)]
        public static extern IntPtr LoadLibrary(string lpFileName);

        [DllImport("kernel32")]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);
#else
        // Change the DllImport as needed
        [DllImport("revoke")]
        public static extern void StringCopy(IntPtr dest, IntPtr src);

        [DllImport("revoke")]
        public static extern void revoke_callbacks(int count, IntPtr[] callbacks, IntPtr[] names);
#endif
        enum ObjType
        {
            VOID,
            INT,
            FLOAT,
            DOUBLE,
            STRING,
            OBJECT,

            BYTE_ARRAY,
            INT_ARRAY,
            FLOAT_ARRAY,
            DOUBLE_ARRAY,
            OBJECT_ARRAY
        }


        // Get a real (typed) reference to the managed objects referenced 
        // by 'nativeRef'
        private static T As<T>(NativeRef nativeRef)
        {
            if (nativeRef == NativeRef.Zero)
                return default(T);
            return (T)GCHandle.FromIntPtr(nativeRef).Target;
        }

        // Wrap a C# object in a native pointer. Will not be garbage collected
        // until the handle is released.
        private static NativeRef ToNative<T>(T obj)
        {
            return GCHandle.ToIntPtr(GCHandle.Alloc(obj));
        }

        // The Interface. All methods tagged with 'Expose' will be wrapped
        // in delegates and passes as a list down to the native code, so they
        // can be called from there.
        // We use the 'NativeRef' alias to indicate an IntPtr that must
        // reference a managed object.
        // When we use raw IntPtr we try to document what it's C/C++
        // equivalent type is expected to be.

        [Expose]
        // Get an object from a C# array
        public static NativeRef GetArrayValue(NativeRef selfPtr, int offset)
        {
            var self = As<Object[]>(selfPtr);
            Object result = self.GetValue(offset);
            return ToNative(result);
        }

        [Expose]
        // Set an object into a C# array
        public static void SetArrayValue(NativeRef selfPtr, int offset, NativeRef argPtr)
        {
            var self = As<Object[]>(selfPtr);
            var obj = As<Object>(argPtr);
            self.SetValue(obj, offset);
        }

        [Expose]
        public static float GetArrayValueFloat(NativeRef selfPtr, int offset)
        {
            return As<float[]>(selfPtr)[offset];
        }

        [Expose]
        public static void SetArrayValueFloat(NativeRef selfPtr, int offset, float arg)
        {
            As<float[]>(selfPtr)[offset] = arg;
        }

        [Expose]
        public static int GetArrayValueInt(NativeRef selfPtr, int offset)
        {
            return As<int[]>(selfPtr)[offset];
        }

        [Expose]
        public static void SetArrayValueInt(NativeRef selfPtr, int offset, int arg)
        {
            As<int[]>(selfPtr)[offset] = arg;
        }

        [Expose]
        public static void GetManyVal(NativeRef selfPtr, int offset, int count, IntPtr target)
        {
            var self = As<Object>(selfPtr);
            var elementType = self.GetType().GetElementType();
            TypeCode t = Type.GetTypeCode(elementType);
            switch (t)
            {
                case TypeCode.Single:
                    Marshal.Copy((Single[])self, offset, target, count);
                    break;
                case TypeCode.Int32:
                    Marshal.Copy((Int32[])self, offset, target, count);
                    break;
            }
        }

        [Expose]
        public static void SetManyVal(NativeRef selfPtr, int offset, int count, IntPtr target)
        {
            var self = As<Object>(selfPtr);
            var elementType = self.GetType().GetElementType();
            TypeCode t = Type.GetTypeCode(elementType);
            switch (t)
            {
                case TypeCode.Single:
                    Marshal.Copy(target, (Single[])self, offset, count);
                    break;
                case TypeCode.Int32:
                    Marshal.Copy(target, (Int32[])self, offset, count);
                    break;
            }
        }

        [Expose]
        public static void SetProperty(string fieldName, NativeRef selfPtr, NativeRef value)
        {
            var self = As<Object>(selfPtr);
            var arg = As<Object>(value);
            var typ = (self is Type) ? (Type)self : self.GetType();
            var prop = typ.GetProperty(fieldName);
            if (prop != null)
            {
                prop.SetValue(self, arg);
            }
            else
            {
                var field = typ.GetField(fieldName);
                field.SetValue(self, arg);
            }
        }

        [Expose]
        public static NativeRef GetProperty(string fieldName, NativeRef selfPtr)
        {
            var self = As<Object>(selfPtr);
            var typ = (self is Type) ? (Type)self : self.GetType();
            var prop = typ.GetProperty(fieldName);

            if (prop != null)
            {
                return ToNative(prop.GetValue(self));
            }
            else
            {
                var field = typ.GetField(fieldName);
                return ToNative(field.GetValue(self));
            }
        }

        [Expose]
        public static NativeRef MethodCall(NativeRef boundMethod, NativeRef selfPtr, int argCount, IntPtr argsPtr)
        {
            var method = As<MethodInfo>(boundMethod);
            var self = As<Object>(selfPtr);

            Console.WriteLine(self);
            Console.WriteLine(method);

            var objs = new Object[argCount];

            if (argCount > 0)
            {
                var args = new IntPtr[argCount];
                Marshal.Copy(argsPtr, args, 0, argCount);
                for (int i = 0; i < argCount; i++)
                {
                    objs[i] = As<Object>(args[i]);
                }
            }

            return ToNative(method.Invoke(self, objs));
        }

        [Expose]
        public static NativeRef NamedCall(string methodName, NativeRef selfPtr, int argCount, IntPtr argsPtr)
        {
            Object self = As<Object>(selfPtr);

            var objs = new Object[argCount];
            var argTypes = new Type[argCount];

            if (argCount > 0)
            {
                var args = new IntPtr[argCount];
                Marshal.Copy(argsPtr, args, 0, argCount);
                for (int i = 0; i < argCount; i++)
                {
                    objs[i] = As<Object>(args[i]);
                    argTypes[i] = objs[i].GetType();
                }
            }

            var typ = (self is Type) ? (Type)self : self.GetType();


            if (methodName == "new")
            {
                var method = typ.GetConstructor(argTypes);
                return ToNative(method.Invoke(objs));
            }
            else
            {
                var method = typ.GetMethod(methodName, argTypes);
                return ToNative(method.Invoke(self, objs));
            }
        }

        [Expose]
        public static NativeRef GetType(string name)
        {
            var ptr = Type.GetType(name);
            return ToNative(ptr);
        }

        [Expose]
        public static int GetMemberCount(NativeRef obj)
        {
            var type = As<Type>(obj);
            return type.GetMembers().Length;
        }

        [Expose]
        public static NativeRef GetTypes(int count, IntPtr /* char** */ charpArray)
        {
            IntPtr[] intPtrArray = new IntPtr[count];
            Type[] typeArray = new Type[count];
            Marshal.Copy(charpArray, intPtrArray, 0, count);

            for (int i = 0; i < count; i++)
            {
                String typeName = Marshal.PtrToStringAnsi(intPtrArray[i]);
                typeArray[i] = Type.GetType(typeName);

            }
            return ToNative(typeArray);
        }

        [Expose]
        public static NativeRef ResolveMethod(string methodName, NativeRef classType, NativeRef argTypes)
        {
            var obj = As<Object>(classType);
            var types = As<Type[]>(argTypes);

            var t = (obj is Type) ? (Type)obj : obj.GetType();
            MethodInfo method = null;
            foreach (var m in t.GetMethods())
            {
                if (methodName == m.Name && m.IsGenericMethod)
                {
                    method = m;
                    break;
                }
            }

            if (method.IsGenericMethod)
            {
                method = method.MakeGenericMethod(types);
            }
            return ToNative(method);
        }

        [Expose]
        public static int GetTypeID(NativeRef ptr, IntPtr sizePtr)
        {
            var obj = As<Object>(ptr);
            //Object obj = GCHandle.FromIntPtr(ptr).Target;
            int size = 0;

            Type at = obj.GetType().GetElementType();

            if (at != null)
            {
                // This is an array
                var objs = (Object[])obj;
                size = objs.Length;
            }
            else if (obj is String)
            {
                size = ((String)obj).Length;
            }
            if (sizePtr != IntPtr.Zero)
                Marshal.WriteInt32(sizePtr, size);

            TypeCode t = Type.GetTypeCode(obj.GetType());

            switch (t)
            {
                case TypeCode.Int32:
                case TypeCode.Int16:
                    return (int)ObjType.INT;
                case TypeCode.Double:
                    return (int)ObjType.DOUBLE;
                case TypeCode.String:
                    return (int)ObjType.STRING;
            }

            return (int)ObjType.VOID;
        }

        [Expose]
        // Convert a managed object to a native object
        public static int CastTo(int type, NativeRef ptr, IntPtr target)
        {
            var obj = As<Object>(ptr);

            var objType = (ObjType)type;

            switch (objType)
            {
                case ObjType.FLOAT:
                    var floatArr = new[] { (float)obj };
                    Marshal.Copy(floatArr, 0, target, 1);
                    break;
                case ObjType.INT:
                    Marshal.WriteInt32(target, (int)obj);
                    return 1;
                case ObjType.STRING:
                    IntPtr cstring = Marshal.StringToHGlobalAuto((String)obj);
#if WIN
                    Invoke<StringCopy>(revokeLibPtr, target, cstring);
#else
                    StringCopy(target, cstring);
#endif
                    Marshal.FreeHGlobal(cstring);
                    return 1;
            }
            return 0;
        }

        [Expose]
        // Release an object created using ToNative
        public static void Free(NativeRef ptr)
        {
            if(ptr == IntPtr.Zero)
                return;
            var handle = GCHandle.FromIntPtr(ptr);
            handle.Free();
        }

        [Expose]
        public static NativeRef CreateFrom(int type, IntPtr source)
        {
            Object obj = null;
            var objType = (ObjType)type;
            switch (objType)
            {
                case ObjType.FLOAT:
                    var floatArr = new float[1];
                    Marshal.Copy(source, floatArr, 0, 1);
                    obj = (float)floatArr[0];
                    break;
                case ObjType.INT:
                    obj = (Object)Marshal.ReadInt32(source);
                    break;
                case ObjType.STRING:
                    obj = Marshal.PtrToStringAnsi(source);
                    break;
            }
            return GCHandle.ToIntPtr(GCHandle.Alloc(obj));
        }

        [Expose]
        public static int GetMembers(NativeRef objPtr, IntPtr /* MemberInfo* */ target)
        {
            var obj = As<Object>(objPtr);
            var t = (obj is Type) ? (Type)obj : obj.GetType();

            MemberInfo[] members = t.GetMembers();
            const int bs = 8;
            int i = 0;
            foreach (var m in members)
            {
                Marshal.WriteIntPtr(target, i, Marshal.StringToHGlobalAuto(m.Name));
                Marshal.WriteIntPtr(target, i + bs, new IntPtr((int)m.MemberType));
                Marshal.WriteIntPtr(target, i + bs*2, ToNative(m));
                i += bs*3;
            }
            return i/(bs*3);

        }

        // --- Interface end

        public static Delegate ToDelegate(MethodInfo mi)
        {
            if (mi == null) throw new ArgumentNullException("mi");

            Type delegateType;

            List<Type> typeArgs = new List<Type>();
            foreach (var p in mi.GetParameters())
            {
                typeArgs.Add(p.ParameterType);
            }

            // builds a delegate type
            if (mi.ReturnType == typeof(void))
            {
                delegateType = Expression.GetActionType(typeArgs.ToArray());
            }
            else
            {
                typeArgs.Add(mi.ReturnType);
                delegateType = Expression.GetDelegateType(typeArgs.ToArray());
            }

            // creates a binded delegate if target is supplied
            return Delegate.CreateDelegate(delegateType, mi);
        }

        public static void Exit()
        {
#if WIN
            if(revokeLibPtr != IntPtr.Zero)
                FreeLibrary(revokeLibPtr);
            revokeLibPtr = IntPtr.Zero;
#endif
        }

        public static void Init(String dllName)
        {
#if WIN
            revokeLibPtr = LoadLibrary(dllName); // "revoke");
#endif
            List<Delegate> delegates = new List<Delegate>();

            Type t = typeof(Revoke);
            var methods = t.GetMethods();
            foreach (var method in methods)
            {
                var attrs = method.GetCustomAttributes(typeof(Expose), true);
                if (attrs.Length > 0)
                {
                    var dt = ToDelegate(method);
                    delegates.Add(dt);
                }
            }

            var callbacks = new IntPtr[delegates.Count];
            var names = new IntPtr[delegates.Count];
            for (int i = 0; i < delegates.Count; i++)
            {
                callbacks[i] = Marshal.GetFunctionPointerForDelegate(delegates[i]);
                names[i] = Marshal.StringToHGlobalAnsi(delegates[i].GetMethodInfo().Name);
            }
#if WIN
            Invoke<revoke_callbacks>(callbacks.Length, callbacks, names);
#else
            revoke_callbacks(callbacks.Length, callbacks, names);
#endif
        }
    }

} // namespace

