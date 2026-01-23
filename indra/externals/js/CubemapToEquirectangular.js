// Copyright (C) 2016 Jaume Sanchez Elias, http://www.clicktorelease.com
// https://github.com/spite/THREE.CubemapToEquirectangular
//
// Modified by Callum Prentice (callum@lindenlab.com) 2021-07-28 to use 
// external jpeg encoder and write XMP metadata

(function () {
    "use strict";

    var root = this;

    var has_require = typeof require !== "undefined";

    var THREE = root.THREE || (has_require && require("three"));
    if (!THREE) throw new Error("CubemapToEquirectangular requires three.js");

    var vertexShader = `
attribute vec3 position;
attribute vec2 uv;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;

varying vec2 vUv;

void main()  {

    vUv = vec2( 1.- uv.x, uv.y );
    gl_Position = projectionMatrix * modelViewMatrix * vec4( position, 1.0 );

}
`;

    var fragmentShader = `
precision mediump float;

uniform samplerCube map;

varying vec2 vUv;

#define M_PI 3.1415926535897932384626433832795

void main() {

    vec2 uv = vUv;

    float longitude = uv.x * 2. * M_PI - M_PI + M_PI / 2.;
    float latitude = uv.y * M_PI;

    vec3 dir = vec3(
        - sin( longitude ) * sin( latitude ),
        cos( latitude ),
        - cos( longitude ) * sin( latitude )
    );
    normalize( dir );

    gl_FragColor = vec4( textureCube( map, dir ).rgb, 1. );

}
`;

    function CubemapToEquirectangular(renderer, provideCubeCamera, eqr_width, eqr_height) {
        this.width = 1;
        this.height = 1;

        this.renderer = renderer;

        this.material = new THREE.RawShaderMaterial({
            uniforms: {
                map: { type: "t", value: null },
            },
            vertexShader: vertexShader,
            fragmentShader: fragmentShader,
            side: THREE.DoubleSide,
        });

        this.scene = new THREE.Scene();
        this.quad = new THREE.Mesh(new THREE.PlaneBufferGeometry(1, 1), this.material);
        this.scene.add(this.quad);
        this.camera = new THREE.OrthographicCamera(1 / -2, 1 / 2, 1 / 2, 1 / -2, -10000, 10000);

        this.canvas = document.createElement("canvas");
        this.ctx = this.canvas.getContext("2d");

        this.cubeCamera = null;
        this.attachedCamera = null;

        this.setSize(eqr_width, eqr_height);

        var gl = this.renderer.getContext();
        this.cubeMapSize = gl.getParameter(gl.MAX_CUBE_MAP_TEXTURE_SIZE);

        if (provideCubeCamera) {
            this.getCubeCamera(eqr_height);
        }
    }

    CubemapToEquirectangular.prototype.setSize = function (width, height) {
        this.width = width;
        this.height = height;

        this.quad.scale.set(this.width, this.height, 1);

        this.camera.left = this.width / -2;
        this.camera.right = this.width / 2;
        this.camera.top = this.height / 2;
        this.camera.bottom = this.height / -2;

        this.camera.updateProjectionMatrix();

        this.output = new THREE.WebGLRenderTarget(this.width, this.height, {
            minFilter: THREE.LinearFilter,
            magFilter: THREE.LinearFilter,
            wrapS: THREE.ClampToEdgeWrapping,
            wrapT: THREE.ClampToEdgeWrapping,
            format: THREE.RGBAFormat,
            type: THREE.UnsignedByteType,
        });

        this.canvas.width = this.width;
        this.canvas.height = this.height;
    };

    CubemapToEquirectangular.prototype.getCubeCamera = function (size) {
        var options = {
            format: THREE.RGBAFormat,
            magFilter: THREE.LinearFilter,
            minFilter: THREE.LinearFilter,
        };
        const renderTarget = new THREE.WebGLCubeRenderTarget(size, options);
        this.cubeCamera = new THREE.CubeCamera(0.1, 1000, renderTarget);

        return this.cubeCamera;
    };

    CubemapToEquirectangular.prototype.attachCubeCamera = function (camera) {
        this.getCubeCamera();
        this.attachedCamera = camera;
    };

    // http://stackoverflow.com/a/5100158/603983
    CubemapToEquirectangular.prototype.dataURItoBlob = function (dataURI) {
        // convert base64/URLEncoded data component to raw binary data held in a string
        var byteString;
        if (dataURI.split(",")[0].indexOf("base64") >= 0) byteString = atob(dataURI.split(",")[1]);
        else byteString = unescape(dataURI.split(",")[1]);

        // separate out the mime component
        var mimeString = dataURI.split(",")[0].split(":")[1].split(";")[0];

        // write the bytes of the string to a typed array
        var ia = new Uint8Array(byteString.length);
        for (var i = 0; i < byteString.length; i++) {
            ia[i] = byteString.charCodeAt(i);
        }

        return new Blob([ia], { type: mimeString });
    };

    CubemapToEquirectangular.prototype.convert = function (cubeCamera, suggested_filename, xmp_details) {
        this.quad.material.uniforms.map.value = cubeCamera.renderTarget.texture;
        this.renderer.setRenderTarget(this.output);
        this.renderer.render(this.scene, this.camera);

        var pixels = new Uint8Array(4 * this.width * this.height);
        this.renderer.readRenderTargetPixels(this.output, 0, 0, this.width, this.height, pixels);

        var imageData = new ImageData(new Uint8ClampedArray(pixels), this.width, this.height);

        var encoder = new JPEGEncoder(90);
        var jpegURI = encoder.encode(imageData, 90, xmp_details);
        var blob = this.dataURItoBlob(jpegURI);

        var url = URL.createObjectURL(blob);
        var anchor = document.createElement("a");
        anchor.href = url;
        anchor.setAttribute("download", suggested_filename);
        anchor.className = "download-js-link";
        anchor.innerHTML = "downloading...";
        anchor.style.display = "none";
        document.body.appendChild(anchor);
        setTimeout(function () {
            anchor.click();
            document.body.removeChild(anchor);
        }, 1);

        this.renderer.setRenderTarget(null);
    };

    CubemapToEquirectangular.prototype.update = function (camera, scene, suggested_filename, xmp_details) {
        var autoClear = this.renderer.autoClear;
        this.renderer.autoClear = true;
        this.cubeCamera.position.copy(camera.position);
        this.cubeCamera.update(this.renderer, scene);
        this.renderer.autoClear = autoClear;

        this.convert(this.cubeCamera, suggested_filename, xmp_details);
    };

    if (typeof exports !== "undefined") {
        if (typeof module !== "undefined" && module.exports) {
            exports = module.exports = CubemapToEquirectangular;
        }
        exports.CubemapToEquirectangular = CubemapToEquirectangular;
    } else {
        root.CubemapToEquirectangular = CubemapToEquirectangular;
    }
}.call(this));
