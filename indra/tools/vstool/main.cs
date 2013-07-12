// Code about getting running instances visual studio
// was borrowed from 
// http://www.codeproject.com/KB/cs/automatingvisualstudio.aspx


using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using Microsoft.CSharp;

namespace VSTool
{
    // The MessageFilter class comes from:
    // http://msdn.microsoft.com/en-us/library/ms228772(VS.80).aspx
    // It allows vstool to get timing error messages from 
    // visualstudio and handle them.
    public class MessageFilter : IOleMessageFilter
    {
        //
        // Class containing the IOleMessageFilter
        // thread error-handling functions.

        // Start the filter.
        public static void Register()
        {
            IOleMessageFilter newFilter = new MessageFilter(); 
            IOleMessageFilter oldFilter = null; 
            CoRegisterMessageFilter(newFilter, out oldFilter);
        }

        // Done with the filter, close it.
        public static void Revoke()
        {
            IOleMessageFilter oldFilter = null; 
            CoRegisterMessageFilter(null, out oldFilter);
        }

        //
        // IOleMessageFilter functions.
        // Handle incoming thread requests.
        int IOleMessageFilter.HandleInComingCall(int dwCallType, 
          System.IntPtr hTaskCaller, int dwTickCount, System.IntPtr 
          lpInterfaceInfo) 
        {
            //Return the flag SERVERCALL_ISHANDLED.
            return 0;
        }

        // Thread call was rejected, so try again.
        int IOleMessageFilter.RetryRejectedCall(System.IntPtr 
          hTaskCallee, int dwTickCount, int dwRejectType)
        {
            if (dwRejectType == 2)
            // flag = SERVERCALL_RETRYLATER.
            {
                // Retry the thread call immediately if return >=0 & 
                // <100.
                return 99;
            }
            // Too busy; cancel call.
            return -1;
        }

        int IOleMessageFilter.MessagePending(System.IntPtr hTaskCallee, 
          int dwTickCount, int dwPendingType)
        {
            //Return the flag PENDINGMSG_WAITDEFPROCESS.
            return 2; 
        }

        // Implement the IOleMessageFilter interface.
        [DllImport("Ole32.dll")]
        private static extern int 
          CoRegisterMessageFilter(IOleMessageFilter newFilter, out 
          IOleMessageFilter oldFilter);
    }

    [ComImport(), Guid("00000016-0000-0000-C000-000000000046"), 
    InterfaceTypeAttribute(ComInterfaceType.InterfaceIsIUnknown)]
    interface IOleMessageFilter 
    {
        [PreserveSig]
        int HandleInComingCall( 
            int dwCallType, 
            IntPtr hTaskCaller, 
            int dwTickCount, 
            IntPtr lpInterfaceInfo);

        [PreserveSig]
        int RetryRejectedCall( 
            IntPtr hTaskCallee, 
            int dwTickCount,
            int dwRejectType);

        [PreserveSig]
        int MessagePending( 
            IntPtr hTaskCallee, 
            int dwTickCount,
            int dwPendingType);
    }

    class ViaCOM
    {
        public static object GetProperty(object from_obj, string prop_name)
        {
            try
            {
                Type objType = from_obj.GetType();
                return objType.InvokeMember(
                    prop_name,
                    BindingFlags.GetProperty, null,
                    from_obj,
                    null);
            }
            catch (Exception e)
            {
                Console.WriteLine("Error getting property: \"{0}\"", prop_name);
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        public static object SetProperty(object from_obj, string prop_name, object new_value)
        {
            try
            {
                object[] args = { new_value };
                Type objType = from_obj.GetType();
                return objType.InvokeMember(
                    prop_name,
                    BindingFlags.DeclaredOnly |
                    BindingFlags.Public |
                    BindingFlags.NonPublic |
                    BindingFlags.Instance |
                    BindingFlags.SetProperty,
                    null,
                    from_obj,
                    args);
            }
            catch (Exception e)
            {
                Console.WriteLine("Error setting property: \"{0}\"", prop_name);
                Console.WriteLine(e.Message);
                throw e;
            }
        }

        public static object CallMethod(object from_obj, string method_name, params object[] args)
        {
            try
            {
                Type objType = from_obj.GetType();
                return objType.InvokeMember(
                    method_name,
                    BindingFlags.DeclaredOnly |
                    BindingFlags.Public |
                    BindingFlags.NonPublic |
                    BindingFlags.Instance |
                    BindingFlags.InvokeMethod,
                    null,
                    from_obj,
                    args);
            }
            catch (Exception e)
            {
                Console.WriteLine("Error calling method \"{0}\"", method_name);
                Console.WriteLine(e.Message);
                throw e;
            }
        }
    };

    /// <summary>
	/// The main entry point class for VSTool.
	/// </summary>
    class VSToolMain
    {
        #region Interop imports
        [DllImport("ole32.dll")]  
        public static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable prot); 
 
        [DllImport("ole32.dll")]  
        public static extern int  CreateBindCtx(int reserved, out IBindCtx ppbc);
        #endregion 

        static System.Boolean ignore_case = true;

        static string solution_name = null;
        static bool use_new_vs = false;
        static Hashtable projectDict = new Hashtable();
        static string startup_project = null;
        static string config = null;

        static object dte = null;
        static object solution = null;

        /// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static int Main(string[] args)
		{
            int retVal = 0;
            bool need_save = false;

            try
            {
                parse_command_line(args);

                Console.WriteLine("Editing solution: {0}", solution_name);

                bool found_open_solution = GetDTEAndSolution();

                if (dte == null || solution == null)
                {
                    retVal = 1;
                }
                else
                {
                    MessageFilter.Register();

                    // Walk through all of the projects in the solution
                    // and list the type of each project.
                    foreach (DictionaryEntry p in projectDict)
                    {
                        string project_name = (string)p.Key;
                        string working_dir = (string)p.Value;
                        if (SetProjectWorkingDir(solution, project_name, working_dir))
                        {
                            need_save = true;
                        }
                    }

                    if (config != null)
                    {
                        need_save = SetActiveConfig(config);
                    }

                    if (startup_project != null)
                    {
                        need_save = SetStartupProject(startup_project);
                    }

                    if (need_save)
                    {
                        if (found_open_solution == false)
                        {
                            ViaCOM.CallMethod(solution, "Close", null);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                retVal = 1;
            }
            finally
            {
                if (solution != null)
                {
                    Marshal.ReleaseComObject(solution);
                    solution = null;
                }

                if (dte != null)
                {
                    Marshal.ReleaseComObject(dte);
                    dte = null;
                }

                MessageFilter.Revoke();
            }
            return retVal;
        }

        public static bool parse_command_line(string[] args)
        {
            string options_desc = 
                "--solution <solution_name>   : MSVC solution name. (required)\n" +
                "--use_new_vs                 : Ignore running versions of visual studio.\n" +
                "--workingdir <project> <dir> : Set working dir of a VC project.\n" +
                "--config <config>            : Set the active config for the solution.\n" +
                "--startup <project>          : Set the startup project for the solution.\n";

            try
            {
                // Command line param parsing loop.
                int i = 0;
                for (; i < args.Length; ++i)
                {
                    if ("--solution" == args[i])
                    {
                        if (solution_name != null)
                        {
                            throw new ApplicationException("Found second --solution option");
                        }
                        solution_name = args[++i];
                    }
                    else if ("--use_new_vs" == args[i])
                    {
                        use_new_vs = true;
                    }

                    else if ("--workingdir" == args[i])
                    {
                        string project_name = args[++i];
                        string working_dir = args[++i];
                        projectDict.Add(project_name, working_dir);
                    }
                    else if ("--config" == args[i])
                    {
                        if (config != null)
                        {
                            throw new ApplicationException("Found second --config option");
                        }
                        config = args[++i];
                    }
                    else if ("--startup" == args[i])
                    {
                        if (startup_project != null)
                        {
                            throw new ApplicationException("Found second --startup option");
                        }
                        startup_project = args[++i];
                    }
                    else
                    {
                        throw new ApplicationException("Found unrecognized token on command line: " + args[i]);
                    }
                }

                if (solution_name == null)
                {
                    throw new ApplicationException("The --solution option is required.");
                }
            }
            catch(ApplicationException e)
            {

                Console.WriteLine("Oops! " + e.Message);
                Console.Write("Command line:");
                foreach (string arg in args)
                {
                    Console.Write(" " + arg);
                }
                Console.Write("\n\n");
                Console.WriteLine("VSTool command line usage");
                Console.Write(options_desc);
                throw e;
            }
            return true;
        }

        public static bool GetDTEAndSolution()
        {
            bool found_open_solution = true;

            Console.WriteLine("Looking for existing VisualStudio instance...");

            // Get an instance of the currently running Visual Studio .NET IDE.
            // dte = (EnvDTE.DTE)System.Runtime.InteropServices.Marshal.GetActiveObject("VisualStudio.DTE.7.1");
            string full_solution_name = System.IO.Path.GetFullPath(solution_name);
            if (false == use_new_vs)
            {
                dte = GetIDEInstance(full_solution_name);
            }

            if (dte == null)
            {
                try
                {
                    Console.WriteLine("  Didn't find open solution, starting new background VisualStudio instance...");
                    Console.WriteLine("  Reading .sln file version...");
                    string version = GetSolutionVersion(full_solution_name);

                    Console.WriteLine("  Using version: {0}...", version);
                    string progid = GetVSProgID(version);

                    Type objType = Type.GetTypeFromProgID(progid);
                    dte = System.Activator.CreateInstance(objType);
                    Console.WriteLine("  Reading solution: \"{0}\"", full_solution_name);

                    solution = ViaCOM.GetProperty(dte, "Solution");
                    object[] openArgs = { full_solution_name };
                    ViaCOM.CallMethod(solution, "Open", openArgs);
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.Message);
                    Console.WriteLine("Quitting do to error opening: {0}", full_solution_name);
                    solution = null;
                    dte = null;
                    return found_open_solution;
                }
                found_open_solution = false;
            }

            if (solution == null)
            {
                solution = ViaCOM.GetProperty(dte, "Solution");
            }

            return found_open_solution;
        }

        /// <summary>
        /// Get the DTE object for the instance of Visual Studio IDE that has 
        /// the specified solution open.
        /// </summary>
        /// <param name="solutionFile">The absolute filename of the solution</param>
        /// <returns>Corresponding DTE object or null if no such IDE is running</returns>
        public static object GetIDEInstance( string solutionFile )
        {
            Hashtable runningInstances = GetIDEInstances( true );
            IDictionaryEnumerator enumerator = runningInstances.GetEnumerator();

            while ( enumerator.MoveNext() )
            {
                try
                {
                    object ide = enumerator.Value;
                    if (ide != null)
                    {
                        object sol = ViaCOM.GetProperty(ide, "Solution");
                        if (0 == string.Compare((string)ViaCOM.GetProperty(sol, "FullName"), solutionFile, ignore_case))
                        {
                            return ide;
                        }
                    }
                } 
                catch{}
            }

            return null;
        }

        /// <summary>
        /// Get a table of the currently running instances of the Visual Studio .NET IDE.
        /// </summary>
        /// <param name="openSolutionsOnly">Only return instances that have opened a solution</param>
        /// <returns>A hashtable mapping the name of the IDE in the running object table to the corresponding DTE object</returns>
        public static Hashtable GetIDEInstances( bool openSolutionsOnly )
        {
            Hashtable runningIDEInstances = new Hashtable();
            Hashtable runningObjects = GetRunningObjectTable();

            IDictionaryEnumerator rotEnumerator = runningObjects.GetEnumerator();
            while ( rotEnumerator.MoveNext() )
            {
                string candidateName = (string) rotEnumerator.Key;
                if (!candidateName.StartsWith("!VisualStudio.DTE"))
                    continue;

                object ide = rotEnumerator.Value;
                if (ide == null)
                    continue;

                if (openSolutionsOnly)
                {
                    try
                    {
                        object sol = ViaCOM.GetProperty(ide, "Solution");
                        string solutionFile = (string)ViaCOM.GetProperty(sol, "FullName");
                        if (solutionFile != String.Empty)
                        {
                            runningIDEInstances[ candidateName ] = ide;
                        }
                    } 
                    catch {}
                }
                else
                {
                    runningIDEInstances[ candidateName ] = ide;
                }                       
            }
            return runningIDEInstances;
        }

        /// <summary>
        /// Get a snapshot of the running object table (ROT).
        /// </summary>
        /// <returns>A hashtable mapping the name of the object in the ROT to the corresponding object</returns>
        [STAThread]
        public static Hashtable GetRunningObjectTable()
        {
            Hashtable result = new Hashtable();

            int numFetched = 0;
            IRunningObjectTable runningObjectTable;   
            IEnumMoniker monikerEnumerator;
            IMoniker[] monikers = new IMoniker[1];

            GetRunningObjectTable(0, out runningObjectTable);    
            runningObjectTable.EnumRunning(out monikerEnumerator);
            monikerEnumerator.Reset();          
            
            while (monikerEnumerator.Next(1, monikers, new IntPtr(numFetched)) == 0)
            {     
                IBindCtx ctx;
                CreateBindCtx(0, out ctx);     
                    
                string runningObjectName;
                monikers[0].GetDisplayName(ctx, null, out runningObjectName);

                object runningObjectVal;  
                runningObjectTable.GetObject( monikers[0], out runningObjectVal); 

                result[ runningObjectName ] = runningObjectVal;
            } 

            return result;
        }

        public static string GetSolutionVersion(string solutionFullFileName) 
        {
            string version;
            System.IO.StreamReader solutionStreamReader = null;
            string firstLine;
            string format;
            
            try
            {
                solutionStreamReader = new System.IO.StreamReader(solutionFullFileName);
                do
                {
                    firstLine = solutionStreamReader.ReadLine();
                }
                while (firstLine == "");
                
                format = firstLine.Substring(firstLine.LastIndexOf(" ")).Trim();
        
                switch(format)
                {
                    case "7.00":
                        version = "VC70";
                        break;

                    case "8.00":
                        version = "VC71";
                        break;

                    case "9.00":
                        version = "VC80";
                        break;
                
                    case "10.00":
                        version = "VC90";
                        break;

                    case "11.00":
                        version = "VC100";
                        break;

                    default:
                        throw new ApplicationException("Unknown .sln version: " + format);
                }
            }
            finally
            {
                if(solutionStreamReader != null) 
                {
                    solutionStreamReader.Close();
                }
            }
            
            return version;
        }

        public static string GetVSProgID(string version)
        {
            string progid = null;
            switch(version)
            {
                case "VC70":
                    progid = "VisualStudio.DTE.7";
                    break;

                case "VC71":
                    progid = "VisualStudio.DTE.7.1";
                    break;

                case "VC80":
                    progid = "VisualStudio.DTE.8.0";
                    break;
                
                case "VC90":
                    progid = "VisualStudio.DTE.9.0";
                    break;

                case "VC100":
                    progid = "VisualStudio.DTE.10.0";
                    break;

                default:
                    throw new ApplicationException("Can't handle VS version: " + version);
            }

            return progid;
        }

        public static bool SetProjectWorkingDir(object sol, string project_name, string working_dir)
        {
            bool made_change = false;
            Console.WriteLine("Looking for project {0}...", project_name);
            try
            {
                object prjs = ViaCOM.GetProperty(sol, "Projects");
                object count = ViaCOM.GetProperty(prjs, "Count");
                for(int i = 1; i <= (int)count; ++i)
                {
                    object[] prjItemArgs = { (object)i };
                    object prj = ViaCOM.CallMethod(prjs, "Item", prjItemArgs);
                    string name = (string)ViaCOM.GetProperty(prj, "Name");
                    if (0 == string.Compare(name, project_name, ignore_case))
                    {
                        Console.WriteLine("Found project: {0}", project_name);
                        Console.WriteLine("Setting working directory");

                        string full_project_name = (string)ViaCOM.GetProperty(prj, "FullName");
                        Console.WriteLine(full_project_name);

                        // *NOTE:Mani Thanks to incompatibilities between different versions of the 
                        // VCProjectEngine.dll assembly, we can't cast the objects recevied from the DTE to
                        // the VCProjectEngine types from a different version than the one built 
                        // with. ie, VisualStudio.DTE.7.1 objects can't be converted in a project built 
                        // in VS 8.0. To avoid this problem, we can use the com object interfaces directly, 
                        // without the type casting. Its tedious code, but it seems to work.

                        // oCfgs should be assigned to a 'Project.Configurations' collection.
                        object oCfgs = ViaCOM.GetProperty(ViaCOM.GetProperty(prj, "Object"), "Configurations");

                        // oCount will be assigned to the number of configs present in oCfgs.
                        object oCount = ViaCOM.GetProperty(oCfgs, "Count");

                        for (int cfgIndex = 1; cfgIndex <= (int)oCount; ++cfgIndex)
                        {
                            object[] itemArgs = {(object)cfgIndex};
                            object oCfg = ViaCOM.CallMethod(oCfgs, "Item", itemArgs);
                            object oDebugSettings = ViaCOM.GetProperty(oCfg, "DebugSettings");
                            ViaCOM.SetProperty(oDebugSettings, "WorkingDirectory", (object)working_dir);
                        }

                        break;
                    }
                }
                made_change = true;
            }
            catch( Exception e )
            {
                Console.WriteLine(e.Message);
                Console.WriteLine("Failed to set working dir for project, {0}.", project_name);
            }

            return made_change;
        }

        public static bool SetStartupProject(string startup_project)
        {
            bool result = false;
            try
            {
                // You need the 'unique name of the project to set StartupProjects.
                // find the project by generic name.
                Console.WriteLine("Trying to set \"{0}\" to the startup project", startup_project);
                object prjs = ViaCOM.GetProperty(solution, "Projects");
                object count = ViaCOM.GetProperty(prjs, "Count");
                for (int i = 1; i <= (int)count; ++i)
                {
                    object[] itemArgs = { (object)i };
                    object prj = ViaCOM.CallMethod(prjs, "Item", itemArgs);
                    object prjName = ViaCOM.GetProperty(prj, "Name");
                    if (0 == string.Compare((string)prjName, startup_project, ignore_case))
                    {
                        object solBuild = ViaCOM.GetProperty(solution, "SolutionBuild");
                        ViaCOM.SetProperty(solBuild, "StartupProjects", ViaCOM.GetProperty(prj, "UniqueName"));
                        Console.WriteLine("  Success!");
                        result = true;
                        break;
                    }
                }

                if (result == false)
                {
                    Console.WriteLine("  Could not find project \"{0}\" in the solution.", startup_project);
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("  Failed to set the startup project!");
                Console.WriteLine(e.Message);
            }
            return result;
        }

        public static bool SetActiveConfig(string config)
        {
            bool result = false;
            try
            {
                Console.WriteLine("Trying to set active config to \"{0}\"", config);
                object solBuild = ViaCOM.GetProperty(solution, "SolutionBuild");
                object solCfgs = ViaCOM.GetProperty(solBuild, "SolutionConfigurations");
                object[] itemArgs = { (object)config };
                object solCfg = ViaCOM.CallMethod(solCfgs, "Item", itemArgs);
                ViaCOM.CallMethod(solCfg, "Activate", null);
                Console.WriteLine("  Success!");
                result = true;
            }
            catch (Exception e)
            {
                Console.WriteLine("  Failed to set \"{0}\" as the active config.", config);
                Console.WriteLine(e.Message);
            }
            return result;
        }
    }
}
