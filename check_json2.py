import json

path = r"D:\Projects\Sanctuary\Sanctuary Maps\Map Editor\map-editor v0.16\SanctuaryMapEditor_Data\Maps\Pandemonium Isthmus\Pandemonium Isthmus.sanmap"
with open(path, 'r', encoding='utf-8') as f:
    data = json.load(f)

expected_floats = [
    "heightTransition", "fadeDistance", "fadeStartDistance", "fogAnisotropy", "fogAttenuationDistance",
    "fogBaseHeight", "fogMaximumDistance", "fogMaximumHeight", "globalWindDirection", "globalWindSpeed",
    "waterLevelMax", "deepWaterMin", "deepWaterMax", "waterWindSpeed", "waterWindDirection", 
    "waterWindShoreWavesRemap", "waterShoreDepthOffset", "waterShoreDepthStrength", 
    "waterShoreDistanceOffset", "waterShoreDistanceStrength", "backgroundFogIntensity",
    "backgroundFogRange", "backgroundFogMinimum", "backgroundSkyColorIntensity", "backgroundColorIntensity",
    "backgroundColorFadeoutRange", "backgroundColorFadeoutPower", "heightFogIntensity", "heightFogStart",
    "heightFogEnd", "heightFogPower", "linearFogIntensity", "linearFogStart", "linearFogEnd",
    "linearFogPower", "linearFogCameraIntensity", "linearFogCameraStart", "linearFogCameraEnd",
    "sunRA", "sunDA", "sunIntensity", "sunTemperature", "sunAngularDiameter", "sunVolumetricsMultiplier",
    "sunVolumetricsShadowDimer", "skylightIntensity", "skylightTemperature", "exposure", "exposureCompensation",
    "skyboxRotation", "skyboxExposure", "skyboxMultiplier", "skyboxLuxValue", "height"
]

for field in expected_floats:
    if field in data:
        t = type(data[field])
        if t is not float:
            print(f"Mismatch: {field} is {t}")
