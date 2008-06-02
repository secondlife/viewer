// Code about getting running instances visual studio
// was borrowed from 
// http://www.codeproject.com/KB/cs/automatingvisualstudio.aspx


using System;
using System.Collections;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using Microsoft.CSharp;

namespace VSTool
{
    class ViaCOM
    {
        public static object GetProperty(object from_obj, string prop_name)
        {
            Type objType = from_obj.GetType();
            return objType.InvokeMember(
                prop_name,
                BindingFlags.GetProperty, null,
                from_obj,
                null);
        }

        public static object SetProperty(object from_obj, string prop_name, object new_value)
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

        public static object CallMethod(object from_obj, string method_name, params object[] args)
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


        /// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
            bool need_save = false;

            parse_command_line(args);

            Console.WriteLine("Opening solution: {0}", solution_name);

            bool found_open_solution = true;

            Console.WriteLine("Looking for existing VisualStudio instance...");

            // Get an instance of the currently running Visual Studio .NET IDE.
            object dte = null;

            // dte = (EnvDTE.DTE)System.Runtime.InteropServices.Marshal.GetActiveObject("VisualStudio.DTE.7.1");
            string full_solution_name = System.IO.Path.GetFullPath(solution_name);
            if (false == use_new_vs)
            {
                dte = GetIDEInstance(full_solution_name);
            }

            object sol = null;

            if (dte == null)
            {
                try
                {
                    Console.WriteLine("  Didn't find open solution, now opening new VisualStudio instance...");
                    Console.WriteLine("  Reading .sln file version...");
                    string version = GetSolutionVersion(full_solution_name);

                    Console.WriteLine("  Opening VS version: {0}...", version);
                    string progid = GetVSProgID(version);

                    Type objType = Type.GetTypeFromProgID(progid);
                    dte = System.Activator.CreateInstance(objType);
    
                    Console.WriteLine("  Opening solution...");

                    sol = ViaCOM.GetProperty(dte, "Solution");
                    object[] openArgs = {full_solution_name};
                    ViaCOM.CallMethod(sol, "Open", openArgs);
                }
                catch( Exception e )
                {
                    Console.WriteLine(e.Message);
                    Console.WriteLine("Quitting do error opening: {0}", full_solution_name);
                    return;
                }
                found_open_solution = false;
            }

            if (sol == null)
            {
                sol = ViaCOM.GetProperty(dte, "Solution");
            }

            // Walk through all of the projects in the solution
            // and list the type of each project.
            foreach(DictionaryEntry p in projectDict)
            {
                string project_name = (string)p.Key;
                string working_dir = (string)p.Value;
                if(SetProjectWorkingDir(sol, project_name, working_dir))
                {
                    need_save = true;
                }
            }

            if(config != null)
            {
                try
                {
                    object solBuild = ViaCOM.GetProperty(sol, "SolutionBuild");
                    object solCfgs = ViaCOM.GetProperty(solBuild, "SolutionConfigurations");
                    object[] itemArgs = { (object)config };
                    object solCfg = ViaCOM.CallMethod(solCfgs, "Item", itemArgs);
                    ViaCOM.CallMethod(solCfg, "Activate", null);
                    need_save = true;
                }
                catch( Exception e )
                {
                    Console.WriteLine(e.Message);
                }
            }

            if (startup_project != null)
            {
                try
                {
                    // You need the 'unique name of the project to set StartupProjects.
                    // find the project by generic name.
                    object prjs = ViaCOM.GetProperty(sol, "Projects");
                    object count = ViaCOM.GetProperty(prjs, "Count");
                    for(int i = 1; i <= (int)count; ++i)
                    {
                        object[] itemArgs = { (object)i };
                        object prj = ViaCOM.CallMethod(prjs, "Item", itemArgs);
                        object prjName = ViaCOM.GetProperty(prj, "Name");
                        if (0 == string.Compare((string)prjName, startup_project, ignore_case))
                        {
                            object solBuild = ViaCOM.GetProperty(sol, "SolutionBuild");
                            ViaCOM.SetProperty(solBuild, "StartupProjects", ViaCOM.GetProperty(prj, "UniqueName"));
                            need_save = true;
                            break;
                        }
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.Message);
                }
            }

            if(need_save)
            {
                if(found_open_solution == false)
                {
                    ViaCOM.CallMethod(sol, "Close", null);
                }
                Console.WriteLine("Finished!");
            }
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
                        // with out the type casting. Its tedious code, but it seems to work.

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
                Console.WriteLine("Failed settings working dir for project, {0}.", project_name);
            }

            return made_change;
        }
    }
}
