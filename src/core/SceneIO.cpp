#include "core/SceneIO.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "core/Box.h"
#include "core/Cylinder.h"
#include "core/Plane.h"
#include "core/Sphere.h"

namespace tinyray {

namespace {

QJsonObject vecToJson(const Vec3& value)
{
    QJsonObject object;
    object["x"] = value.x;
    object["y"] = value.y;
    object["z"] = value.z;
    return object;
}

Vec3 vecFromJson(const QJsonObject& object, const Vec3& fallback)
{
    return Vec3(
        object.value("x").toDouble(fallback.x),
        object.value("y").toDouble(fallback.y),
        object.value("z").toDouble(fallback.z));
}

QString materialTypeToString(MaterialType type)
{
    switch (type) {
    case MaterialType::Metal:
        return "Metal";
    case MaterialType::Glass:
        return "Glass";
    case MaterialType::Emissive:
        return "Emissive";
    case MaterialType::Diffuse:
    default:
        return "Diffuse";
    }
}

MaterialType materialTypeFromString(const QString& value)
{
    if (value.compare(QStringLiteral("Metal"), Qt::CaseInsensitive) == 0) {
        return MaterialType::Metal;
    }
    if (value.compare(QStringLiteral("Glass"), Qt::CaseInsensitive) == 0) {
        return MaterialType::Glass;
    }
    if (value.compare(QStringLiteral("Emissive"), Qt::CaseInsensitive) == 0) {
        return MaterialType::Emissive;
    }
    return MaterialType::Diffuse;
}

QJsonObject materialToJson(const Material& material)
{
    QJsonObject object;
    object["type"] = materialTypeToString(material.type);
    object["albedo"] = vecToJson(material.albedo);
    object["roughness"] = material.roughness;
    object["refractiveIndex"] = material.refractiveIndex;
    object["emissionColor"] = vecToJson(material.emissionColor);
    object["emissionStrength"] = material.emissionStrength;
    object["emissiveColor"] = vecToJson(material.emissionColor);
    object["emissiveIntensity"] = material.emissionStrength;
    return object;
}

Material materialFromJson(const QJsonObject& object)
{
    Material material;
    material.type = materialTypeFromString(object.value("type").toString("Diffuse"));
    material.albedo = vecFromJson(object.value("albedo").toObject(), material.albedo);
    material.roughness = object.value("roughness").toDouble(material.roughness);
    material.refractiveIndex = object.value("refractiveIndex").toDouble(material.refractiveIndex);
    const QJsonObject emissionColorObject = object.contains("emissiveColor")
        ? object.value("emissiveColor").toObject()
        : object.value("emissionColor").toObject();
    material.emissionColor = vecFromJson(emissionColorObject, material.emissionColor);
    material.emissionStrength = object.contains("emissiveIntensity")
        ? object.value("emissiveIntensity").toDouble(material.emissionStrength)
        : object.value("emissionStrength").toDouble(material.emissionStrength);
    return material;
}

void setError(QString* errorMessage, const QString& text)
{
    if (errorMessage) {
        *errorMessage = text;
    }
}

} // namespace

bool SceneIO::saveToFile(const Scene& scene,
                         const RenderSettings& settings,
                         const QString& fileName,
                         QString* errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setError(errorMessage, QStringLiteral("Cannot open scene file for writing."));
        return false;
    }

    QJsonObject root;
    root["format"] = "TinyRayStudioScene";
    root["version"] = 1;

    QJsonObject camera;
    camera["position"] = vecToJson(scene.camera.position);
    camera["lookAt"] = vecToJson(scene.camera.lookAt);
    camera["up"] = vecToJson(scene.camera.up);
    camera["fov"] = scene.camera.fieldOfViewYDegrees;
    camera["aspectRatio"] = scene.camera.aspectRatio;
    root["camera"] = camera;
    root["selectedObjectId"] = scene.selectedObjectId;
    root["softShadowsEnabled"] = scene.softShadowsEnabled;
    root["areaLightSamples"] = scene.areaLightSamples;

    QJsonObject bloom;
    bloom["enabled"] = scene.bloomSettings.enabled;
    bloom["exposure"] = scene.bloomSettings.exposure;
    bloom["threshold"] = scene.bloomSettings.threshold;
    bloom["strength"] = scene.bloomSettings.strength;
    bloom["blurPassCount"] = scene.bloomSettings.blurPassCount;
    root["bloom"] = bloom;

    QJsonObject renderSettings;
    renderSettings["width"] = settings.width;
    renderSettings["height"] = settings.height;
    renderSettings["samplesPerPixel"] = settings.samplesPerPixel;
    renderSettings["maxDepth"] = settings.maxDepth;
    renderSettings["threadCount"] = settings.threadCount;
    root["renderSettings"] = renderSettings;

    QJsonArray objects;
    for (const auto& object : scene.objects) {
        if (const auto* sphere = dynamic_cast<const Sphere*>(object.get())) {
            QJsonObject item;
            item["type"] = "Sphere";
            item["id"] = sphere->id();
            item["center"] = vecToJson(sphere->center);
            item["radius"] = sphere->radius;
            item["material"] = materialToJson(sphere->material);
            objects.append(item);
        } else if (const auto* box = dynamic_cast<const Box*>(object.get())) {
            QJsonObject item;
            item["type"] = "Box";
            item["id"] = box->id();
            item["center"] = vecToJson(box->center);
            item["size"] = vecToJson(box->size);
            item["material"] = materialToJson(box->material);
            objects.append(item);
        } else if (const auto* cylinder = dynamic_cast<const Cylinder*>(object.get())) {
            QJsonObject item;
            item["type"] = "Cylinder";
            item["id"] = cylinder->id();
            item["center"] = vecToJson(cylinder->center);
            item["radius"] = cylinder->radius;
            item["height"] = cylinder->height;
            item["material"] = materialToJson(cylinder->material);
            objects.append(item);
        } else if (const auto* plane = dynamic_cast<const Plane*>(object.get())) {
            QJsonObject item;
            item["type"] = "Plane";
            item["id"] = plane->id();
            item["point"] = vecToJson(plane->point);
            item["normal"] = vecToJson(plane->normal);
            item["material"] = materialToJson(plane->material);
            objects.append(item);
        }
    }
    root["objects"] = objects;

    QJsonArray lights;
    for (const Light& light : scene.lights) {
        QJsonObject item;
        item["type"] = light.type == LightType::Area ? "AreaLight" : "PointLight";
        item["position"] = vecToJson(light.position);
        item["normal"] = vecToJson(light.normal);
        item["width"] = light.width;
        item["height"] = light.height;
        item["color"] = vecToJson(light.color);
        item["intensity"] = light.intensity;
        lights.append(item);
    }
    root["lights"] = lights;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool SceneIO::loadFromFile(const QString& fileName,
                           Scene& scene,
                           RenderSettings& settings,
                           QString* errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, QStringLiteral("Cannot open scene file for reading."));
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        setError(errorMessage, QStringLiteral("Invalid JSON scene file."));
        return false;
    }

    const QJsonObject root = document.object();
    if (root.value("format").toString() != "TinyRayStudioScene") {
        setError(errorMessage, QStringLiteral("Unsupported scene file format."));
        return false;
    }

    Scene loadedScene;
    const QJsonObject camera = root.value("camera").toObject();
    loadedScene.camera.position = vecFromJson(camera.value("position").toObject(), loadedScene.camera.position);
    loadedScene.camera.lookAt = vecFromJson(camera.value("lookAt").toObject(), loadedScene.camera.lookAt);
    loadedScene.camera.up = vecFromJson(camera.value("up").toObject(), loadedScene.camera.up);
    loadedScene.camera.fieldOfViewYDegrees = camera.value("fov").toDouble(loadedScene.camera.fieldOfViewYDegrees);
    loadedScene.camera.aspectRatio = camera.value("aspectRatio").toDouble(loadedScene.camera.aspectRatio);
    loadedScene.selectedObjectId = root.value("selectedObjectId").toInt(-1);
    loadedScene.softShadowsEnabled = root.value("softShadowsEnabled").toBool(loadedScene.softShadowsEnabled);
    loadedScene.areaLightSamples = root.value("areaLightSamples").toInt(loadedScene.areaLightSamples);

    const QJsonObject bloom = root.value("bloom").toObject();
    loadedScene.bloomSettings.enabled = bloom.value("enabled").toBool(loadedScene.bloomSettings.enabled);
    loadedScene.bloomSettings.exposure = bloom.value("exposure").toDouble(loadedScene.bloomSettings.exposure);
    loadedScene.bloomSettings.threshold = bloom.value("threshold").toDouble(loadedScene.bloomSettings.threshold);
    loadedScene.bloomSettings.strength = bloom.value("strength").toDouble(loadedScene.bloomSettings.strength);
    loadedScene.bloomSettings.blurPassCount = bloom.value("blurPassCount").toInt(loadedScene.bloomSettings.blurPassCount);

    const QJsonObject renderSettings = root.value("renderSettings").toObject();
    settings.width = renderSettings.value("width").toInt(settings.width);
    settings.height = renderSettings.value("height").toInt(settings.height);
    settings.samplesPerPixel = renderSettings.value("samplesPerPixel").toInt(settings.samplesPerPixel);
    settings.maxDepth = renderSettings.value("maxDepth").toInt(settings.maxDepth);
    settings.threadCount = renderSettings.value("threadCount").toInt(settings.threadCount);

    const QJsonArray objects = root.value("objects").toArray();
    for (const QJsonValue& value : objects) {
        const QJsonObject item = value.toObject();
        const QString type = item.value("type").toString();
        const Material material = materialFromJson(item.value("material").toObject());

        if (type == "Sphere") {
            Sphere sphere(
                vecFromJson(item.value("center").toObject(), Vec3(0.0, 0.0, 0.0)),
                item.value("radius").toDouble(1.0),
                material);
            sphere.setId(item.value("id").toInt(sphere.id()));
            loadedScene.addSphere(sphere);
        } else if (type == "Box") {
            Box box(
                vecFromJson(item.value("center").toObject(), Vec3(0.0, 0.0, 0.0)),
                vecFromJson(item.value("size").toObject(), Vec3(1.0, 1.0, 1.0)),
                material);
            box.setId(item.value("id").toInt(box.id()));
            loadedScene.addBox(box);
        } else if (type == "Cylinder") {
            Cylinder cylinder(
                vecFromJson(item.value("center").toObject(), Vec3(0.0, 0.0, 0.0)),
                item.value("radius").toDouble(0.5),
                item.value("height").toDouble(1.0),
                material);
            cylinder.setId(item.value("id").toInt(cylinder.id()));
            loadedScene.addCylinder(cylinder);
        } else if (type == "Plane") {
            Plane plane(
                vecFromJson(item.value("point").toObject(), Vec3(0.0, 0.0, 0.0)),
                vecFromJson(item.value("normal").toObject(), Vec3(0.0, 1.0, 0.0)),
                material);
            plane.setId(item.value("id").toInt(plane.id()));
            loadedScene.addPlane(plane);
        }
    }

    const QJsonArray lights = root.value("lights").toArray();
    for (const QJsonValue& value : lights) {
        const QJsonObject item = value.toObject();
        const QString type = item.value("type").toString("PointLight");
        if (type == "AreaLight") {
            loadedScene.addLight(Light::area(
                vecFromJson(item.value("position").toObject(), Vec3(0.0, 4.0, 2.0)),
                vecFromJson(item.value("normal").toObject(), Vec3(0.0, -1.0, 0.0)),
                item.value("width").toDouble(2.0),
                item.value("height").toDouble(2.0),
                vecFromJson(item.value("color").toObject(), Vec3(1.0, 1.0, 1.0)),
                item.value("intensity").toDouble(1.0)));
        } else {
            loadedScene.addLight(Light(
                vecFromJson(item.value("position").toObject(), Vec3(0.0, 4.0, 2.0)),
                vecFromJson(item.value("color").toObject(), Vec3(1.0, 1.0, 1.0)),
                item.value("intensity").toDouble(1.0)));
        }
    }

    scene = loadedScene;
    return true;
}

} // namespace tinyray
