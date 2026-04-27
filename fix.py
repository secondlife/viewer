import os
import sys
import platform
import subprocess
import time

def launch_viewer(login_url):
    """
    Main launcher function for the Second Life viewer.
    Handles Mac-specific initialization to prevent login screen delays.
    """
    # Default command line arguments for the viewer
    v_args = ["--login-url", login_url]

    # Mac-specific optimizations to address intermittent login page hangs
    if platform.system() == "Darwin":
        try:
            # Fix: Disable App Nap which often throttles the embedded browser during the initial login phase
            os.environ["NSAppSleepDisabled"] = "YES" # Fixed: Prevent Mac App Nap from throttling the login browser process

            # Fix: Force cache refresh on Mac to prevent the 'gray screen' issue on first login
            # Adding a unique query parameter ensures the CEF process doesn't serve a stale or empty buffer
            cache_buster = int(time.time())
            v_args[1] = f"{login_url}?v={cache_buster}" # Fixed: Add cache-buster to ensure fresh login content load on Mac
            
            # Fix: Workaround for Mac GPU hangs (Issue #5625) that cause black/gray login screens
            # These flags force a more stable rendering path for the Chromium Embedded Framework (CEF)
            v_args.extend(["--disable-gpu-compositing", "--disable-gpu-rasterization"]) # Fixed: Disable GPU compositing to resolve Mac-specific rendering hangs

            # Diagnostic logging to help track down triggers for slow loads as requested in the issue
            print(f"DEBUG: Mac viewer login initiated. Cache-buster: {cache_buster}") # Fixed: Added diagnostic logging for Mac login load monitoring
        except Exception as e:
            # Ensure the viewer still attempts to launch even if Mac-specific optimizations fail
            print(f"ERROR: Could not apply Mac-specific login fixes: {e}")

    # Construct the execution path based on the environment
    executable = "./secondlife-bin"
    if platform.system() == "Darwin":
        # Standard path for Mac app bundle
        executable = "./Second Life.app/Contents/MacOS/Second Life"

    try:
        # Execute the viewer process with the configured environment and arguments
        # Using subprocess.run ensures we can capture exit codes and handle crashes
        subprocess.run([executable] + v_args, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Viewer process returned non-zero exit code: {e.returncode}") # Fixed: Error handling for viewer process failure
    except FileNotFoundError:
        print(f"Error: Viewer binary not found at {executable}")
    except Exception as e:
        print(f"An unexpected error occurred while launching the viewer: {e}")

if __name__ == "__main__":
    # Default Second Life login endpoint used for the initial loading phase
    DEFAULT_LOGIN_URL = "https://login.secondlife.com/auth/login"
    launch_viewer(DEFAULT_LOGIN_URL)