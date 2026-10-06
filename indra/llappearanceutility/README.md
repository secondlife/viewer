# llappearanceutility

This directory contains the appearance utility employed by the bake service to process avatar textures and handle various other appearance-related functions. Please note that this utility is currently configured to build only on Linux systems.

## Prerequisites

Ensure the following prerequisites are met before building and testing the utility:

- **X11 Display**: Ensure an X11 display is available, either virtual or physical, with the `DISPLAY` environment variable set appropriately (e.g., `DISPLAY=:1`).
- **Linux Build Environment**: Confirm that the Second Life Viewer can be built on your Linux system.

## Testing

Given the constraints with GitHub runners, we instead utilize Behavior-Driven Development (BDD) tests to evaluate the `llappearanceutility` through the bake service. For local development, test inputs and their expected outputs are provided below.

### Test Commands

1. **Baking Textures**:
    ```sh
    appearance-utility-bin --agent-id de494a4f-f01a-47a4-98cf-c94ef9ecca38 --texture /viewer/indra/llappearanceutility/tests/texture.llsd.binary > /viewer/indra/llappearanceutility/tests/texture.llsd.output
    ```

2. **Generating Bake Parameters**:
    ```sh
    appearance-utility-bin --agent-id de494a4f-f01a-47a4-98cf-c94ef9ecca38 --params /viewer/indra/llappearanceutility/tests/params.xml > /viewer/indra/llappearanceutility/tests/params.xml.output
    ```

3. **Calculating Joint Offsets**:
    ```sh
    appearance-utility-bin --agent-id de494a4f-f01a-47a4-98cf-c94ef9ecca38 --joint-offsets /viewer/indra/llappearanceutility/tests/joint-offsets.xml > /viewer/indra/llappearanceutility/tests/joint-offsets.xml.output
    ```

These commands will execute the utility with specific test inputs and direct the output to corresponding files for validation.