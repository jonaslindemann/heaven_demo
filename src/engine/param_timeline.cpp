#include "param_timeline.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>

using namespace demo;

namespace {

constexpr float kPi = 3.14159265358979f;

bool parseEase(const std::string &name, Ease &ease)
{
    static const std::map<std::string, Ease> names = {
        {"linear", Ease::Linear}, {"step", Ease::Step}, {"smooth", Ease::Smooth}, {"sine", Ease::Sine},
        {"in", Ease::In},         {"out", Ease::Out},   {"cubic", Ease::Cubic},   {"spline", Ease::Spline}};
    auto it = names.find(name);
    if (it == names.end())
        return false;
    ease = it->second;
    return true;
}

bool parseValue(const nlohmann::json &json, glm::vec4 &value, int &components)
{
    if (json.is_number())
    {
        value = glm::vec4(json.get<float>(), 0.0f, 0.0f, 0.0f);
        components = 1;
        return true;
    }
    if (json.is_array() && json.size() >= 1 && json.size() <= 4)
    {
        value = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        for (size_t i = 0; i < json.size(); ++i)
        {
            if (!json[i].is_number())
                return false;
            value[int(i)] = json[i].get<float>();
        }
        components = int(json.size());
        return true;
    }
    return false;
}

} // namespace

float demo::applyEase(Ease ease, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    switch (ease)
    {
    case Ease::Linear:
    case Ease::Spline:
        return t;
    case Ease::Step:
        return t < 1.0f ? 0.0f : 1.0f;
    case Ease::Smooth:
        return t * t * (3.0f - 2.0f * t);
    case Ease::Sine:
        return 0.5f - 0.5f * std::cos(kPi * t);
    case Ease::In:
        return t * t;
    case Ease::Out:
        return 1.0f - (1.0f - t) * (1.0f - t);
    case Ease::Cubic:
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }
    return t;
}

bool ParamTimeline::loadFile(const std::string &path, std::string *error)
{
    auto fail = [&](const std::string &message) {
        if (error)
            *error = path + ": " + message;
        return false;
    };

    std::ifstream file(path);
    if (!file)
        return fail("cannot open");

    nlohmann::json json;
    try
    {
        json = nlohmann::json::parse(file, nullptr, true, true); // allow comments
    }
    catch (const nlohmann::json::parse_error &e)
    {
        return fail(e.what());
    }

    // Parse into a new timeline so an error keeps the current contents
    ParamTimeline loaded;

    if (json.contains("markers"))
    {
        for (const auto &marker : json["markers"])
        {
            if (!marker.contains("bar") || !marker["bar"].is_number() || !marker.contains("name"))
                return fail("markers need a numeric \"bar\" and a \"name\"");
            loaded.addMarker(marker["bar"].get<double>(), marker["name"].get<std::string>());
        }
    }

    if (json.contains("params"))
    {
        for (const auto &[name, keys] : json["params"].items())
        {
            if (!keys.is_array())
                return fail("parameter \"" + name + "\" must be a list of keys");
            for (const auto &key : keys)
            {
                if (!key.is_array() || key.size() < 2 || key.size() > 3 || !key[0].is_number())
                    return fail("parameter \"" + name + "\": keys are [bar, value] or [bar, value, \"ease\"]");

                glm::vec4 value;
                int components = 1;
                if (!parseValue(key[1], value, components))
                    return fail("parameter \"" + name + "\": a value is a number or a list of 1-4 numbers");

                Ease ease = Ease::Linear;
                if (key.size() == 3 && (!key[2].is_string() || !parseEase(key[2].get<std::string>(), ease)))
                    return fail("parameter \"" + name +
                                "\": unknown easing (use linear, step, smooth, sine, in, out, cubic, spline)");

                loaded.setKey(name, key[0].get<double>(), value, ease, components);
            }
        }
    }

    loaded.m_path = path;
    std::error_code ec;
    loaded.m_fileTime = std::filesystem::last_write_time(path, ec);
    *this = std::move(loaded);
    return true;
}

bool ParamTimeline::reloadIfChanged(std::string *error)
{
    if (m_path.empty())
        return false;

    std::error_code ec;
    const auto time = std::filesystem::last_write_time(m_path, ec);
    if (ec || time == m_fileTime)
        return false;

    // Remember the time even on failure, so a broken file is reported once per save
    m_fileTime = time;
    const std::string path = m_path;
    return loadFile(path, error);
}

void ParamTimeline::clear()
{
    m_params.clear();
    m_markers.clear();
}

void ParamTimeline::setKey(const std::string &name, double bar, glm::vec4 value, Ease ease, int components)
{
    const Key key{bar, value, ease, std::clamp(components, 1, 4)};
    auto &keys = m_params[name];
    auto it = std::lower_bound(keys.begin(), keys.end(), bar, [](const Key &k, double b) { return k.bar < b; });
    if (it != keys.end() && it->bar == bar)
        *it = key;
    else
        keys.insert(it, key);
}

void ParamTimeline::setKey(const std::string &name, double bar, float value, Ease ease)
{
    setKey(name, bar, glm::vec4(value, 0.0f, 0.0f, 0.0f), ease, 1);
}

int ParamTimeline::components(std::string_view name) const
{
    auto found = m_params.find(name);
    if (found == m_params.end())
        return 0;
    int result = 0;
    for (const auto &key : found->second)
        result = std::max(result, key.components);
    return result;
}

void ParamTimeline::addMarker(double bar, std::string name)
{
    auto it = std::upper_bound(m_markers.begin(), m_markers.end(), bar,
                               [](double b, const Marker &m) { return b < m.bar; });
    m_markers.insert(it, Marker{bar, std::move(name)});
}

bool ParamTimeline::has(std::string_view name) const
{
    return m_params.find(name) != m_params.end();
}

glm::vec4 ParamTimeline::vec4(std::string_view name, double bar, glm::vec4 fallback) const
{
    auto found = m_params.find(name);
    if (found == m_params.end() || found->second.empty())
        return fallback;

    const auto &keys = found->second;
    auto next = std::upper_bound(keys.begin(), keys.end(), bar, [](double b, const Key &k) { return b < k.bar; });
    if (next == keys.begin())
        return keys.front().value;
    if (next == keys.end())
        return keys.back().value;

    const Key &a = *(next - 1);
    const Key &b = *next;
    const float t = float((bar - a.bar) / (b.bar - a.bar));
    if (b.ease != Ease::Spline)
        return glm::mix(a.value, b.value, applyEase(b.ease, t));

    // Cubic Hermite segment. The tangent at a key between two spline segments is
    // the Catmull-Rom slope (per bar) through its neighbours; anywhere else it is zero.
    auto tangent = [&](size_t k) {
        if (k == 0 || k + 1 >= keys.size() || keys[k].ease != Ease::Spline || keys[k + 1].ease != Ease::Spline)
            return glm::vec4(0.0f);
        return (keys[k + 1].value - keys[k - 1].value) / float(keys[k + 1].bar - keys[k - 1].bar);
    };
    const size_t ia = size_t(next - keys.begin()) - 1;
    const float h = float(b.bar - a.bar);
    const float t2 = t * t, t3 = t2 * t;
    return (2.0f * t3 - 3.0f * t2 + 1.0f) * a.value + (t3 - 2.0f * t2 + t) * h * tangent(ia) +
           (-2.0f * t3 + 3.0f * t2) * b.value + (t3 - t2) * h * tangent(ia + 1);
}

glm::vec3 ParamTimeline::vec3(std::string_view name, double bar, glm::vec3 fallback) const
{
    return has(name) ? glm::vec3(vec4(name, bar)) : fallback;
}

float ParamTimeline::value(std::string_view name, double bar, float fallback) const
{
    return has(name) ? vec4(name, bar).x : fallback;
}

double ParamTimeline::integral(std::string_view name, double fromBar, double toBar, float fallback, double step) const
{
    if (toBar <= fromBar)
        return 0.0;
    if (!has(name))
        return double(fallback) * (toBar - fromBar);
    const int steps = std::max(1, int(std::ceil((toBar - fromBar) / step)));
    const double h = (toBar - fromBar) / steps;
    double sum = 0.5 * (value(name, fromBar) + value(name, toBar));
    for (int i = 1; i < steps; ++i)
        sum += value(name, fromBar + i * h);
    return sum * h;
}

int ParamTimeline::markerIndexAt(double bar) const
{
    auto it = std::upper_bound(m_markers.begin(), m_markers.end(), bar,
                               [](double b, const Marker &m) { return b < m.bar; });
    return int(it - m_markers.begin()) - 1;
}
