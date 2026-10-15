/* ********************************************************************************** */
/*                                                                                    */
/*                                                      :::::::::: :::::::::   :::::: */
/*   GlbLoader.cpp                                     :+:        :+:    :+: :+:    : */
/*                                                    +:+        +:+    +:+ +:+       */
/*   By: flaviobc <github.com/Flaviobc>              :#::+::#   +#++:++#+  +#+        */
/*                                                  +#+        +#+    +#+ +#+         */
/*   Created: 2026/09/26          by flaviobc      #+#        #+#    #+# #+#    #+#   */
/*   Updated: 2026/09/26          by flaviobc     ###        #########   ########     */
/*                                                                                    */
/* ********************************************************************************** */

#include "GlbLoader.hpp"

#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <utility>
#include <variant>

namespace
{
constexpr std::uint32_t kGlbMagic = 0x46546C67;
constexpr std::uint32_t kGlbVersion = 2;
constexpr std::uint32_t kJsonChunk = 0x4E4F534A;
constexpr std::uint32_t kBinaryChunk = 0x004E4942;

struct JsonValue;
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::map<std::string, JsonValue, std::less<>>;

struct JsonValue
{
	using Storage = std::variant<std::nullptr_t, bool, double, std::string,
	                             JsonArray, JsonObject>;
	Storage value;

	[[nodiscard]] const JsonObject *object() const
	{
		return std::get_if<JsonObject>(&value);
	}

	[[nodiscard]] const JsonArray *array() const
	{
		return std::get_if<JsonArray>(&value);
	}

	[[nodiscard]] const std::string *string() const
	{
		return std::get_if<std::string>(&value);
	}

	[[nodiscard]] const double *number() const
	{
		return std::get_if<double>(&value);
	}
};

class JsonParser
{
  public:
	explicit JsonParser(const std::string &text) : text_(text)
	{
	}

	[[nodiscard]] bool parse(JsonValue &value, std::string &error)
	{
		skipWhitespace();
		if (!parseValue(value, error))
		{
			return false;
		}
		skipWhitespace();
		if (position_ != text_.size())
		{
			error = "unexpected data after JSON document";
			return false;
		}
		return true;
	}

  private:
	void skipWhitespace()
	{
		while (position_ < text_.size() &&
		       std::isspace(static_cast<unsigned char>(text_[position_])))
		{
			++position_;
		}
	}

	[[nodiscard]] bool parseValue(JsonValue &value, std::string &error)
	{
		if (position_ == text_.size())
		{
			error = "unexpected end of JSON";
			return false;
		}
		switch (text_[position_])
		{
		case '{':
			return parseObject(value, error);
		case '[':
			return parseArray(value, error);
		case '"':
		{
			std::string string;
			if (!parseString(string, error))
			{
				return false;
			}
			value.value = std::move(string);
			return true;
		}
		case 't':
			return parseLiteral("true", true, value, error);
		case 'f':
			return parseLiteral("false", false, value, error);
		case 'n':
			return parseLiteral("null", nullptr, value, error);
		default:
			return parseNumber(value, error);
		}
	}

	[[nodiscard]] bool parseObject(JsonValue &value, std::string &error)
	{
		JsonObject object;
		++position_;
		skipWhitespace();
		if (consume('}'))
		{
			value.value = std::move(object);
			return true;
		}
		while (true)
		{
			std::string key;
			if (!parseString(key, error))
			{
				return false;
			}
			skipWhitespace();
			if (!consume(':'))
			{
				error = "missing colon in JSON object";
				return false;
			}
			skipWhitespace();
			JsonValue member;
			if (!parseValue(member, error))
			{
				return false;
			}
			object.emplace(std::move(key), std::move(member));
			skipWhitespace();
			if (consume('}'))
			{
				value.value = std::move(object);
				return true;
			}
			if (!consume(','))
			{
				error = "missing comma in JSON object";
				return false;
			}
			skipWhitespace();
		}
	}

	[[nodiscard]] bool parseArray(JsonValue &value, std::string &error)
	{
		JsonArray array;
		++position_;
		skipWhitespace();
		if (consume(']'))
		{
			value.value = std::move(array);
			return true;
		}
		while (true)
		{
			JsonValue element;
			if (!parseValue(element, error))
			{
				return false;
			}
			array.push_back(std::move(element));
			skipWhitespace();
			if (consume(']'))
			{
				value.value = std::move(array);
				return true;
			}
			if (!consume(','))
			{
				error = "missing comma in JSON array";
				return false;
			}
			skipWhitespace();
		}
	}

	[[nodiscard]] bool parseString(std::string &string, std::string &error)
	{
		if (!consume('"'))
		{
			error = "expected JSON string";
			return false;
		}
		while (position_ < text_.size())
		{
			const char character = text_[position_++];
			if (character == '"')
			{
				return true;
			}
			if (character != '\\')
			{
				string += character;
				continue;
			}
			if (position_ == text_.size())
			{
				error = "unterminated JSON escape";
				return false;
			}
			const char escape = text_[position_++];
			switch (escape)
			{
			case '"':
				string += '"';
				break;
			case '\\':
				string += '\\';
				break;
			case '/':
				string += '/';
				break;
			case 'b':
				string += '\b';
				break;
			case 'f':
				string += '\f';
				break;
			case 'n':
				string += '\n';
				break;
			case 'r':
				string += '\r';
				break;
			case 't':
				string += '\t';
				break;
			default:
				error = "unsupported JSON escape";
				return false;
			}
		}
		error = "unterminated JSON string";
		return false;
	}

	template <typename T>
	[[nodiscard]] bool parseLiteral(const char *literal, T literalValue,
	                                JsonValue &value, std::string &error)
	{
		const std::size_t length = std::char_traits<char>::length(literal);
		if (text_.compare(position_, length, literal) != 0)
		{
			error = "invalid JSON literal";
			return false;
		}
		position_ += length;
		value.value = literalValue;
		return true;
	}

	[[nodiscard]] bool parseNumber(JsonValue &value, std::string &error)
	{
		const std::size_t start = position_;
		if (position_ < text_.size() && text_[position_] == '-')
		{
			++position_;
		}
		while (position_ < text_.size() &&
		       std::isdigit(static_cast<unsigned char>(text_[position_])))
		{
			++position_;
		}
		if (position_ < text_.size() && text_[position_] == '.')
		{
			++position_;
			while (position_ < text_.size() &&
			       std::isdigit(static_cast<unsigned char>(text_[position_])))
			{
				++position_;
			}
		}
		if (position_ < text_.size() &&
		    (text_[position_] == 'e' || text_[position_] == 'E'))
		{
			++position_;
			if (position_ < text_.size() &&
			    (text_[position_] == '+' || text_[position_] == '-'))
			{
				++position_;
			}
			while (position_ < text_.size() &&
			       std::isdigit(static_cast<unsigned char>(text_[position_])))
			{
				++position_;
			}
		}
		if (start == position_)
		{
			error = "invalid JSON value";
			return false;
		}
		try
		{
			value.value = std::stod(text_.substr(start, position_ - start));
		}
		catch (const std::exception &)
		{
			error = "invalid JSON number";
			return false;
		}
		return true;
	}

	[[nodiscard]] bool consume(char character)
	{
		if (position_ == text_.size() || text_[position_] != character)
		{
			return false;
		}
		++position_;
		return true;
	}

	const std::string &text_;
	std::size_t position_ = 0;
};

const JsonValue *member(const JsonObject &object, const char *name)
{
	const auto found = object.find(name);
	return found == object.end() ? nullptr : &found->second;
}

bool readIndex(const JsonValue &value, std::size_t &index)
{
	const double *number = value.number();
	// size_t's maximum can round up when converted to double. Use the exact
	// exclusive power-of-two bound before any floating-to-integer conversion.
	if (!number || !std::isfinite(*number) || *number < 0.0 ||
	    *number >= std::ldexp(1.0, std::numeric_limits<std::size_t>::digits) ||
	    std::floor(*number) != *number)
	{
		return false;
	}
	index = static_cast<std::size_t>(*number);
	return true;
}

bool readBinaryFile(const std::filesystem::path &path, std::vector<char> &data,
                    std::string &error)
{
	std::ifstream input(path, std::ios::binary | std::ios::ate);
	if (!input)
	{
		error = "could not open file";
		return false;
	}
	const std::streamsize size = input.tellg();
	if (size < 0)
	{
		error = "could not determine file size";
		return false;
	}
	data.resize(static_cast<std::size_t>(size));
	input.seekg(0);
	if (!input.read(data.data(), size))
	{
		error = "could not read file";
		return false;
	}
	return true;
}

bool readU32(const std::vector<char> &data, std::size_t offset,
             std::uint32_t &value)
{
	if (offset > data.size() || data.size() - offset < sizeof(value))
	{
		return false;
	}
	std::memcpy(&value, data.data() + offset, sizeof(value));
	return true;
}

// A GLB is a header followed by a JSON chunk and, for this asset, a BIN chunk.
bool readGlbChunks(const std::vector<char> &data, std::string &json,
                   std::vector<char> &binary, std::string &error)
{
	std::uint32_t magic = 0;
	std::uint32_t version = 0;
	std::uint32_t declaredLength = 0;
	std::uint32_t chunkLength = 0;
	std::uint32_t chunkType = 0;
	if (!readU32(data, 0, magic) || !readU32(data, 4, version) ||
	    !readU32(data, 8, declaredLength) || !readU32(data, 12, chunkLength) ||
	    !readU32(data, 16, chunkType))
	{
		error = "file is shorter than the GLB header";
		return false;
	}
	if (magic != kGlbMagic || version != kGlbVersion)
	{
		error = "file is not a glTF 2.0 binary asset";
		return false;
	}
	if (declaredLength != data.size())
	{
		error = "GLB declared length does not match file size";
		return false;
	}
	if (chunkType != kJsonChunk || chunkLength > data.size() - 20U)
	{
		error = "GLB does not begin with a valid JSON chunk";
		return false;
	}
	json.assign(data.data() + 20, chunkLength);

	const std::size_t binaryHeaderOffset = 20U + chunkLength;
	if (!readU32(data, binaryHeaderOffset, chunkLength) ||
	    !readU32(data, binaryHeaderOffset + sizeof(chunkLength), chunkType) ||
	    chunkType != kBinaryChunk ||
	    chunkLength > data.size() - binaryHeaderOffset - 8U)
	{
		error = "GLB does not contain a valid BIN chunk";
		return false;
	}
	binary.assign(
	    data.begin() + static_cast<std::ptrdiff_t>(binaryHeaderOffset + 8U),
	    data.begin() +
	        static_cast<std::ptrdiff_t>(binaryHeaderOffset + 8U + chunkLength));
	return true;
}

struct BufferView
{
	// A bufferView selects a contiguous byte range from the GLB BIN chunk.
	std::size_t byteOffset = 0;
	std::size_t byteLength = 0;
	std::size_t byteStride = 0;
};

struct Accessor
{
	// An accessor describes typed elements inside one bufferView.
	std::size_t bufferViewIndex = 0;
	std::size_t byteOffset = 0;
	std::size_t count = 0;
	std::uint32_t componentType = 0;
	std::string type;
};

bool readMemberIndex(const JsonObject &object, const char *name,
                     std::size_t &index)
{
	const JsonValue *value = member(object, name);
	return value && readIndex(*value, index);
}

bool readBufferViews(const JsonArray &values, std::vector<BufferView> &views,
                     std::string &error)
{
	for (const JsonValue &value : values)
	{
		const JsonObject *object = value.object();
		std::size_t bufferIndex = 0;
		BufferView view;
		if (!object || !readMemberIndex(*object, "buffer", bufferIndex) ||
		    bufferIndex != 0 ||
		    !readMemberIndex(*object, "byteLength", view.byteLength))
		{
			error =
			    "bufferView must reference GLB buffer 0 and have byteLength";
			return false;
		}
		const JsonValue *byteOffset = member(*object, "byteOffset");
		if (byteOffset && !readIndex(*byteOffset, view.byteOffset))
		{
			error = "bufferView has an invalid byteOffset";
			return false;
		}
		const JsonValue *byteStride = member(*object, "byteStride");
		if (byteStride && !readIndex(*byteStride, view.byteStride))
		{
			error = "bufferView has an invalid byteStride";
			return false;
		}
		views.push_back(view);
	}
	return true;
}

bool readAccessors(const JsonArray &values, std::vector<Accessor> &accessors,
                   std::string &error)
{
	for (const JsonValue &value : values)
	{
		const JsonObject *object = value.object();
		Accessor accessor;
		if (!object ||
		    !readMemberIndex(*object, "bufferView", accessor.bufferViewIndex) ||
		    !readMemberIndex(*object, "count", accessor.count))
		{
			error = "accessor must have bufferView and count";
			return false;
		}
		const JsonValue *componentType = member(*object, "componentType");
		const JsonValue *type = member(*object, "type");
		const double *componentNumber =
		    componentType ? componentType->number() : nullptr;
		const std::string *typeText = type ? type->string() : nullptr;
		if (!componentNumber || !std::isfinite(*componentNumber) ||
		    *componentNumber < 0.0 ||
		    *componentNumber > std::numeric_limits<std::uint32_t>::max() ||
		    std::floor(*componentNumber) != *componentNumber || !typeText)
		{
			error = "accessor must have componentType and type";
			return false;
		}
		accessor.componentType = static_cast<std::uint32_t>(*componentNumber);
		accessor.type = *typeText;
		const JsonValue *byteOffset = member(*object, "byteOffset");
		if (byteOffset && !readIndex(*byteOffset, accessor.byteOffset))
		{
			error = "accessor has an invalid byteOffset";
			return false;
		}
		accessors.push_back(std::move(accessor));
	}
	return true;
}

bool elementOffset(const BufferView &view, const Accessor &accessor,
                   std::size_t elementSize, std::size_t elementIndex,
                   std::size_t binarySize, std::size_t &offset)
{
	const std::size_t stride =
	    view.byteStride == 0 ? elementSize : view.byteStride;
	if (view.byteOffset > binarySize ||
	    view.byteLength > binarySize - view.byteOffset ||
	    accessor.byteOffset > view.byteLength || stride < elementSize ||
	    elementSize > view.byteLength - accessor.byteOffset ||
	    elementIndex >
	        (view.byteLength - accessor.byteOffset - elementSize) / stride)
	{
		return false;
	}
	// BIN offset = bufferView offset + accessor offset + element index *
	// stride.
	offset = view.byteOffset + accessor.byteOffset + elementIndex * stride;
	return true;
}

bool readTexCoords(const JsonObject &attributes, const char *attributeName,
                   const std::vector<Accessor> &accessors,
                   const std::vector<BufferView> &views,
                   const std::vector<char> &binary, std::size_t positionCount,
                   std::vector<engine::assets::StaticMeshTexCoord> &texCoords,
                   std::string &error)
{
	const JsonValue *accessorValue = member(attributes, attributeName);
	if (!accessorValue)
	{
		return true;
	}
	std::size_t accessorIndex = 0;
	if (!readIndex(*accessorValue, accessorIndex) ||
	    accessorIndex >= accessors.size())
	{
		error = std::string(attributeName) + " has an invalid accessor index";
		return false;
	}
	const Accessor &accessor = accessors[accessorIndex];
	if (accessor.bufferViewIndex >= views.size() ||
	    accessor.componentType != 5126 || accessor.type != "VEC2")
	{
		error = std::string(attributeName) + " must use float VEC2 data";
		return false;
	}
	if (accessor.count != positionCount)
	{
		error = std::string(attributeName) + " count must match POSITION count";
		return false;
	}
	const BufferView &view = views[accessor.bufferViewIndex];
	texCoords.reserve(accessor.count);
	for (std::size_t texCoordIndex = 0; texCoordIndex < accessor.count;
	     ++texCoordIndex)
	{
		std::size_t offset = 0;
		if (!elementOffset(view, accessor, sizeof(float) * 2U, texCoordIndex,
		                   binary.size(), offset))
		{
			error =
			    std::string(attributeName) + " accessor exceeds its bufferView";
			return false;
		}
		engine::assets::StaticMeshTexCoord texCoord;
		// UV bytes remain CPU data; no texture is decoded or uploaded here.
		std::memcpy(&texCoord, binary.data() + offset, sizeof(texCoord));
		texCoords.push_back(texCoord);
	}
	return true;
}

bool readSkinAttributes(
    const JsonObject &attributes, const std::vector<Accessor> &accessors,
    const std::vector<BufferView> &views, const std::vector<char> &binary,
    std::size_t positionCount,
    std::vector<engine::assets::StaticMeshJointIndices> &jointIndices,
    std::vector<engine::assets::StaticMeshJointWeights> &jointWeights,
    std::string &error)
{
	const JsonValue *jointsValue = member(attributes, "JOINTS_0");
	const JsonValue *weightsValue = member(attributes, "WEIGHTS_0");
	if (!jointsValue && !weightsValue)
	{
		return true;
	}
	std::size_t jointsAccessorIndex = 0;
	std::size_t weightsAccessorIndex = 0;
	if (!jointsValue || !weightsValue ||
	    !readIndex(*jointsValue, jointsAccessorIndex) ||
	    !readIndex(*weightsValue, weightsAccessorIndex) ||
	    jointsAccessorIndex >= accessors.size() ||
	    weightsAccessorIndex >= accessors.size())
	{
		error =
		    "JOINTS_0 and WEIGHTS_0 must reference valid accessors together";
		return false;
	}
	const Accessor &joints = accessors[jointsAccessorIndex];
	const Accessor &weights = accessors[weightsAccessorIndex];
	if (joints.bufferViewIndex >= views.size() ||
	    joints.componentType != 5121 || joints.type != "VEC4" ||
	    joints.count != positionCount)
	{
		error =
		    "JOINTS_0 must use UNSIGNED_BYTE VEC4 data matching POSITION count";
		return false;
	}
	if (weights.bufferViewIndex >= views.size() ||
	    weights.componentType != 5126 || weights.type != "VEC4" ||
	    weights.count != positionCount)
	{
		error = "WEIGHTS_0 must use FLOAT VEC4 data matching POSITION count";
		return false;
	}
	const BufferView &jointsView = views[joints.bufferViewIndex];
	const BufferView &weightsView = views[weights.bufferViewIndex];
	jointIndices.reserve(positionCount);
	jointWeights.reserve(positionCount);
	for (std::size_t vertexIndex = 0; vertexIndex < positionCount;
	     ++vertexIndex)
	{
		std::size_t jointsOffset = 0;
		std::size_t weightsOffset = 0;
		if (!elementOffset(jointsView, joints, 4U, vertexIndex, binary.size(),
		                   jointsOffset) ||
		    !elementOffset(weightsView, weights, sizeof(float) * 4U,
		                   vertexIndex, binary.size(), weightsOffset))
		{
			error = "skin attribute accessor exceeds its bufferView";
			return false;
		}
		const auto *rawJoints = reinterpret_cast<const std::uint8_t *>(
		    binary.data() + jointsOffset);
		jointIndices.push_back(
		    {rawJoints[0], rawJoints[1], rawJoints[2], rawJoints[3]});
		engine::assets::StaticMeshJointWeights vertexWeights;
		std::memcpy(&vertexWeights, binary.data() + weightsOffset,
		            sizeof(vertexWeights));
		const float weightSum = vertexWeights.first + vertexWeights.second +
		                        vertexWeights.third + vertexWeights.fourth;
		if (!std::isfinite(vertexWeights.first) ||
		    !std::isfinite(vertexWeights.second) ||
		    !std::isfinite(vertexWeights.third) ||
		    !std::isfinite(vertexWeights.fourth) ||
		    std::fabs(weightSum - 1.0F) > 0.001F)
		{
			error = "WEIGHTS_0 values must be finite and normalized";
			return false;
		}
		jointWeights.push_back(vertexWeights);
	}
	return true;
}

bool readStaticMeshSource(const JsonObject &primitive,
                          const std::vector<Accessor> &accessors,
                          const std::vector<BufferView> &views,
                          const std::vector<char> &binary,
                          std::optional<std::size_t> requiredTexCoord,
                          engine::assets::StaticMeshSource &source,
                          std::string &error)
{
	const JsonValue *mode = member(primitive, "mode");
	if (mode)
	{
		std::size_t primitiveMode = 0;
		if (!readIndex(*mode, primitiveMode) || primitiveMode != 4)
		{
			error = "only triangular glTF primitives are supported";
			return false;
		}
	}
	const JsonValue *attributes = member(primitive, "attributes");
	const JsonObject *attributeObject =
	    attributes ? attributes->object() : nullptr;
	std::size_t positionAccessorIndex = 0;
	if (!attributeObject ||
	    !readMemberIndex(*attributeObject, "POSITION", positionAccessorIndex) ||
	    positionAccessorIndex >= accessors.size())
	{
		error = "triangular primitive is missing a POSITION accessor";
		return false;
	}
	const Accessor &positions = accessors[positionAccessorIndex];
	if (positions.bufferViewIndex >= views.size() ||
	    positions.componentType != 5126 || positions.type != "VEC3")
	{
		error = "POSITION must use float VEC3 data";
		return false;
	}
	const BufferView &positionView = views[positions.bufferViewIndex];
	source.positions.reserve(positions.count);
	for (std::size_t positionIndex = 0; positionIndex < positions.count;
	     ++positionIndex)
	{
		std::size_t offset = 0;
		if (!elementOffset(positionView, positions, sizeof(float) * 3U,
		                   positionIndex, binary.size(), offset))
		{
			error = "POSITION accessor exceeds its bufferView";
			return false;
		}
		engine::assets::StaticMeshPosition position;
		// The three little-endian float bytes become one CPU-side position.
		std::memcpy(&position, binary.data() + offset, sizeof(position));
		source.positions.push_back(position);
	}

	const JsonValue *normalAccessorValue = member(*attributeObject, "NORMAL");
	if (normalAccessorValue)
	{
		std::size_t normalAccessorIndex = 0;
		if (!readIndex(*normalAccessorValue, normalAccessorIndex) ||
		    normalAccessorIndex >= accessors.size())
		{
			error = "NORMAL has an invalid accessor index";
			return false;
		}
		const Accessor &normals = accessors[normalAccessorIndex];
		if (normals.bufferViewIndex >= views.size() ||
		    normals.componentType != 5126 || normals.type != "VEC3")
		{
			error = "NORMAL must use float VEC3 data";
			return false;
		}
		if (normals.count != source.positions.size())
		{
			error = "NORMAL count must match POSITION count";
			return false;
		}
		const BufferView &normalView = views[normals.bufferViewIndex];
		source.normals.reserve(normals.count);
		for (std::size_t normalIndex = 0; normalIndex < normals.count;
		     ++normalIndex)
		{
			std::size_t offset = 0;
			if (!elementOffset(normalView, normals, sizeof(float) * 3U,
			                   normalIndex, binary.size(), offset))
			{
				error = "NORMAL accessor exceeds its bufferView";
				return false;
			}
			engine::assets::StaticMeshNormal normal;
			// Normals remain CPU vectors; Rendering will decide whether to
			// upload them.
			std::memcpy(&normal, binary.data() + offset, sizeof(normal));
			source.normals.push_back(normal);
		}
	}
	if (!readTexCoords(*attributeObject, "TEXCOORD_0", accessors, views, binary,
	                   source.positions.size(), source.texCoords0, error) ||
	    !readTexCoords(*attributeObject, "TEXCOORD_1", accessors, views, binary,
	                   source.positions.size(), source.texCoords1, error))
	{
		return false;
	}
	if (!readSkinAttributes(*attributeObject, accessors, views, binary,
	                        source.positions.size(), source.jointIndices,
	                        source.jointWeights, error))
	{
		return false;
	}
	if (requiredTexCoord)
	{
		const std::vector<engine::assets::StaticMeshTexCoord> &selectedUvs =
		    *requiredTexCoord == 0 ? source.texCoords0 : source.texCoords1;
		if (*requiredTexCoord > 1 || selectedUvs.empty())
		{
			error =
			    "baseColorTexture texCoord has no matching TEXCOORD attribute";
			return false;
		}
	}

	const JsonValue *indicesValue = member(primitive, "indices");
	if (!indicesValue)
	{
		if (source.positions.size() % 3U != 0U)
		{
			error = "non-indexed triangle primitive has a "
			        "non-multiple-of-three vertex count";
			return false;
		}
		return true;
	}
	std::size_t indexAccessorIndex = 0;
	if (!readIndex(*indicesValue, indexAccessorIndex) ||
	    indexAccessorIndex >= accessors.size())
	{
		error = "primitive has an invalid index accessor";
		return false;
	}
	const Accessor &indices = accessors[indexAccessorIndex];
	if (indices.bufferViewIndex >= views.size() || indices.type != "SCALAR" ||
	    (indices.componentType != 5121 && indices.componentType != 5123 &&
	     indices.componentType != 5125))
	{
		error = "indices must use unsigned scalar data";
		return false;
	}
	if (indices.count % 3U != 0U)
	{
		error = "triangle primitive has a non-multiple-of-three index count";
		return false;
	}
	const BufferView &indexView = views[indices.bufferViewIndex];
	const std::size_t indexSize = indices.componentType == 5121   ? 1U
	                              : indices.componentType == 5123 ? 2U
	                                                              : 4U;
	source.indices.reserve(indices.count);
	for (std::size_t indexIndex = 0; indexIndex < indices.count; ++indexIndex)
	{
		std::size_t offset = 0;
		if (!elementOffset(indexView, indices, indexSize, indexIndex,
		                   binary.size(), offset))
		{
			error = "index accessor exceeds its bufferView";
			return false;
		}
		std::uint32_t index = 0;
		if (indexSize == 1U)
		{
			index = static_cast<unsigned char>(binary[offset]);
		}
		else if (indexSize == 2U)
		{
			std::uint16_t shortIndex = 0;
			std::memcpy(&shortIndex, binary.data() + offset,
			            sizeof(shortIndex));
			index = shortIndex;
		}
		else
		{
			std::memcpy(&index, binary.data() + offset, sizeof(index));
		}
		// The converted index addresses this primitive's CPU positions vector.
		if (index >= source.positions.size())
		{
			error = "index references a position outside its primitive";
			return false;
		}
		source.indices.push_back(index);
	}
	return true;
}

bool readImages(const JsonArray &values, const std::vector<BufferView> &views,
                const std::vector<char> &binary,
                std::vector<engine::assets::GlbImage> &images,
                std::string &error)
{
	for (const JsonValue &value : values)
	{
		const JsonObject *object = value.object();
		engine::assets::GlbImage image;
		if (!object ||
		    !readMemberIndex(*object, "bufferView", image.bufferViewIndex) ||
		    image.bufferViewIndex >= views.size())
		{
			error = "image must reference a valid internal bufferView";
			return false;
		}
		const JsonValue *mimeType = member(*object, "mimeType");
		const std::string *mimeText = mimeType ? mimeType->string() : nullptr;
		if (!mimeText || *mimeText != "image/png")
		{
			error = "only embedded image/png is supported by this CPU subset";
			return false;
		}
		const JsonValue *name = member(*object, "name");
		const std::string *nameText = name ? name->string() : nullptr;
		image.name = nameText ? *nameText : std::string{};
		image.mimeType = *mimeText;
		const BufferView &view = views[image.bufferViewIndex];
		if (view.byteOffset > binary.size() ||
		    view.byteLength > binary.size() - view.byteOffset)
		{
			error = "image bufferView exceeds the GLB BIN chunk";
			return false;
		}
		image.encodedBytes.assign(
		    reinterpret_cast<const std::uint8_t *>(binary.data() +
			                                       view.byteOffset),
		    reinterpret_cast<const std::uint8_t *>(
		        binary.data() + view.byteOffset + view.byteLength));
		// The document owns both the GLB bytes and the decoded CPU pixels.
		if (!engine::assets::decodePngRgba8(image.encodedBytes,
		                                    image.decodedImage, error))
		{
			error = "embedded PNG image failed to decode: " + error;
			return false;
		}
		images.push_back(std::move(image));
	}
	return true;
}

bool readTextures(const JsonArray &values,
                  const std::vector<engine::assets::GlbImage> &images,
                  std::vector<engine::assets::GlbTexture> &textures,
                  std::string &error)
{
	for (const JsonValue &value : values)
	{
		const JsonObject *object = value.object();
		std::size_t imageIndex = 0;
		if (!object || !readMemberIndex(*object, "source", imageIndex) ||
		    imageIndex >= images.size())
		{
			error = "texture must reference a valid image source";
			return false;
		}
		textures.push_back({imageIndex});
	}
	return true;
}

bool readBaseColor(const JsonObject &materialObject,
                   const std::vector<engine::assets::GlbTexture> &textures,
                   engine::assets::GlbMaterial &material, std::string &error)
{
	// glTF stores the material's CPU-side RGBA base color under this PBR
	// object.
	const JsonValue *pbrValue = member(materialObject, "pbrMetallicRoughness");
	if (!pbrValue)
	{
		return true;
	}
	const JsonObject *pbr = pbrValue->object();
	if (!pbr)
	{
		error = "pbrMetallicRoughness must be an object";
		return false;
	}
	const JsonValue *factorValue = member(*pbr, "baseColorFactor");
	if (!factorValue)
	{
		const JsonValue *textureValue = member(*pbr, "baseColorTexture");
		if (!textureValue)
		{
			return true;
		}
		const JsonObject *textureObject = textureValue->object();
		std::size_t textureIndex = 0;
		if (!textureObject ||
		    !readMemberIndex(*textureObject, "index", textureIndex) ||
		    textureIndex >= textures.size())
		{
			error = "baseColorTexture has an invalid texture index";
			return false;
		}
		std::size_t texCoord = 0;
		const JsonValue *texCoordValue = member(*textureObject, "texCoord");
		if (texCoordValue && !readIndex(*texCoordValue, texCoord))
		{
			error = "baseColorTexture has an invalid texCoord";
			return false;
		}
		if (texCoord > 1)
		{
			error =
			    "baseColorTexture texCoord must be 0 or 1 in this CPU subset";
			return false;
		}
		material.baseColorTexture = {textureIndex, texCoord};
		return true;
	}
	const JsonArray *factor = factorValue->array();
	if (!factor || factor->size() != 4U)
	{
		error = "baseColorFactor must contain exactly four RGBA values";
		return false;
	}
	std::array<float *, 4> channels = {
	    &material.baseColor.red, &material.baseColor.green,
	    &material.baseColor.blue, &material.baseColor.alpha};
	for (std::size_t channelIndex = 0; channelIndex < factor->size();
	     ++channelIndex)
	{
		const double *value = factor->at(channelIndex).number();
		if (!value || !std::isfinite(*value) || *value < 0.0 || *value > 1.0)
		{
			error = "baseColorFactor values must be finite numbers from 0 to 1";
			return false;
		}
		*channels[channelIndex] = static_cast<float>(*value);
	}
	const JsonValue *textureValue = member(*pbr, "baseColorTexture");
	if (!textureValue)
	{
		return true;
	}
	const JsonObject *textureObject = textureValue->object();
	std::size_t textureIndex = 0;
	if (!textureObject ||
	    !readMemberIndex(*textureObject, "index", textureIndex) ||
	    textureIndex >= textures.size())
	{
		error = "baseColorTexture has an invalid texture index";
		return false;
	}
	std::size_t texCoord = 0;
	const JsonValue *texCoordValue = member(*textureObject, "texCoord");
	if (texCoordValue && !readIndex(*texCoordValue, texCoord))
	{
		error = "baseColorTexture has an invalid texCoord";
		return false;
	}
	if (texCoord > 1)
	{
		error = "baseColorTexture texCoord must be 0 or 1 in this CPU subset";
		return false;
	}
	material.baseColorTexture = {textureIndex, texCoord};
	return true;
}

bool readEmissiveColor(const JsonObject &materialObject,
	                      engine::assets::GlbMaterial &material,
	                      std::string &error)
{
	const JsonValue *factorValue = member(materialObject, "emissiveFactor");
	if (!factorValue)
	{
		return true;
	}
	const JsonArray *factor = factorValue->array();
	if (!factor || factor->size() != 3U)
	{
		error = "emissiveFactor must contain exactly three RGB values";
		return false;
	}
	std::array<float *, 3> channels = {
	    &material.emissiveColor.red,
	    &material.emissiveColor.green,
	    &material.emissiveColor.blue,
	};
	for (std::size_t channelIndex = 0; channelIndex < factor->size();
	     ++channelIndex)
	{
		const double *value = factor->at(channelIndex).number();
		if (!value || !std::isfinite(*value))
		{
			error = "emissiveFactor values must be finite numbers";
			return false;
		}
		*channels[channelIndex] = static_cast<float>(*value);
	}
	return true;
}

struct AnimationSamplerReference
{
	std::size_t inputAccessorIndex = 0;
	std::size_t outputAccessorIndex = 0;
	std::optional<engine::assets::GlbAnimationPath> path;
};

bool readAnimationTimes(const Accessor &accessor,
                        const std::vector<BufferView> &views,
                        const std::vector<char> &binary,
                        std::vector<float> &times, std::string &error)
{
	if (accessor.bufferViewIndex >= views.size() ||
	    accessor.componentType != 5126 || accessor.type != "SCALAR" ||
	    accessor.count == 0)
	{
		error = "animation input must use nonempty FLOAT SCALAR data";
		return false;
	}
	const BufferView &view = views[accessor.bufferViewIndex];
	times.reserve(accessor.count);
	for (std::size_t keyIndex = 0; keyIndex < accessor.count; ++keyIndex)
	{
		std::size_t offset = 0;
		if (!elementOffset(view, accessor, sizeof(float), keyIndex, binary.size(),
		                   offset))
		{
			error = "animation input accessor exceeds its bufferView";
			return false;
		}
		float time = 0.0F;
		std::memcpy(&time, binary.data() + offset, sizeof(time));
		if (!std::isfinite(time) || (!times.empty() && time < times.back()))
		{
			error = "animation times must be finite and ordered";
			return false;
		}
		times.push_back(time);
	}
	return true;
}

template <typename Value>
bool readAnimationValues(const Accessor &accessor, const char *expectedType,
	                     const std::vector<BufferView> &views,
	                     const std::vector<char> &binary,
	                     std::size_t expectedCount,
	                     std::vector<Value> &values, std::string &error)
{
	if (accessor.bufferViewIndex >= views.size() || accessor.componentType != 5126 ||
	    accessor.type != expectedType || accessor.count != expectedCount)
	{
		error = std::string("animation output must use FLOAT ") + expectedType +
		        " data with the same count as its input";
		return false;
	}
	const BufferView &view = views[accessor.bufferViewIndex];
	values.reserve(accessor.count);
	for (std::size_t keyIndex = 0; keyIndex < accessor.count; ++keyIndex)
	{
		std::size_t offset = 0;
		if (!elementOffset(view, accessor, sizeof(Value), keyIndex, binary.size(),
		                   offset))
		{
			error = "animation output accessor exceeds its bufferView";
			return false;
		}
		Value value;
		std::memcpy(&value, binary.data() + offset, sizeof(value));
		values.push_back(value);
	}
	return true;
}

bool readAnimations(const JsonArray &values, const std::vector<Accessor> &accessors,
	                const std::vector<BufferView> &views,
	                const std::vector<char> &binary,
	                const std::vector<engine::assets::GlbNode> &nodes,
	                std::vector<engine::assets::GlbAnimationClip> &animations,
	                std::string &error)
{
	for (const JsonValue &value : values)
	{
		const JsonObject *object = value.object();
		const JsonValue *samplersValue = object ? member(*object, "samplers") : nullptr;
		const JsonValue *channelsValue = object ? member(*object, "channels") : nullptr;
		if (!object || !samplersValue || !channelsValue || !samplersValue->array() ||
		    !channelsValue->array())
		{
			error = "animation must contain samplers and channels arrays";
			return false;
		}
		const JsonValue *nameValue = member(*object, "name");
		const std::string *name = nameValue ? nameValue->string() : nullptr;
		engine::assets::GlbAnimationClip clip{
		    .name = name ? *name : std::string{},
		    .durationSeconds = 0.0F,
		    .samplers = {},
		    .channels = {}};
		std::vector<AnimationSamplerReference> references;
		for (const JsonValue &samplerValue : *samplersValue->array())
		{
			const JsonObject *sampler = samplerValue.object();
			AnimationSamplerReference reference;
			if (!sampler || !readMemberIndex(*sampler, "input", reference.inputAccessorIndex) ||
			    !readMemberIndex(*sampler, "output", reference.outputAccessorIndex) ||
			    reference.inputAccessorIndex >= accessors.size() ||
			    reference.outputAccessorIndex >= accessors.size())
			{
				error = "animation sampler has an invalid input or output accessor";
				return false;
			}
			const JsonValue *interpolation = member(*sampler, "interpolation");
			const std::string *interpolationText =
			    interpolation ? interpolation->string() : nullptr;
			if (interpolation && (!interpolationText || *interpolationText != "LINEAR"))
			{
				error = "only LINEAR animation interpolation is supported";
				return false;
			}
			references.push_back(reference);
		}
		for (const JsonValue &channelValue : *channelsValue->array())
		{
			const JsonObject *channel = channelValue.object();
			std::size_t samplerIndex = 0;
			const JsonValue *targetValue = channel ? member(*channel, "target") : nullptr;
			const JsonObject *target = targetValue ? targetValue->object() : nullptr;
			std::size_t nodeIndex = 0;
			const JsonValue *pathValue = target ? member(*target, "path") : nullptr;
			const std::string *path = pathValue ? pathValue->string() : nullptr;
			if (!channel || !readMemberIndex(*channel, "sampler", samplerIndex) ||
			    samplerIndex >= references.size() || !target ||
			    !readMemberIndex(*target, "node", nodeIndex) || nodeIndex >= nodes.size() ||
			    !path)
			{
				error = "animation channel has an invalid sampler or target node";
				return false;
			}
			engine::assets::GlbAnimationPath animationPath;
			if (*path == "translation")
			{
				animationPath = engine::assets::GlbAnimationPath::Translation;
			}
			else if (*path == "rotation")
			{
				animationPath = engine::assets::GlbAnimationPath::Rotation;
			}
			else
			{
				error = "animation channel path must be translation or rotation";
				return false;
			}
			AnimationSamplerReference &reference = references[samplerIndex];
			if (reference.path && *reference.path != animationPath)
			{
				error = "one animation sampler cannot serve different output paths";
				return false;
			}
			reference.path = animationPath;
			clip.channels.push_back({samplerIndex, nodeIndex, animationPath});
		}
		clip.samplers.resize(references.size());
		float firstTime = std::numeric_limits<float>::max();
		float lastTime = std::numeric_limits<float>::lowest();
		for (std::size_t samplerIndex = 0; samplerIndex < references.size(); ++samplerIndex)
		{
			const AnimationSamplerReference &reference = references[samplerIndex];
			if (!reference.path)
			{
				error = "animation sampler is not referenced by a channel";
				return false;
			}
			engine::assets::GlbAnimationSampler &sampler = clip.samplers[samplerIndex];
			sampler.path = *reference.path;
			if (!readAnimationTimes(accessors[reference.inputAccessorIndex], views, binary,
			                        sampler.times, error))
			{
				return false;
			}
			if (*reference.path == engine::assets::GlbAnimationPath::Translation)
			{
				if (!readAnimationValues<engine::assets::GlbAnimationTranslation>(
				        accessors[reference.outputAccessorIndex], "VEC3", views, binary,
				        sampler.times.size(), sampler.translations, error))
				{
					return false;
				}
			}
			else if (!readAnimationValues<engine::assets::GlbAnimationRotation>(
			             accessors[reference.outputAccessorIndex], "VEC4", views, binary,
			             sampler.times.size(), sampler.rotations, error))
			{
				return false;
			}
			firstTime = std::min(firstTime, sampler.times.front());
			lastTime = std::max(lastTime, sampler.times.back());
		}
		clip.durationSeconds = lastTime - firstTime;
		animations.push_back(std::move(clip));
	}
	return true;
}

bool readSkins(const JsonArray &values, const std::vector<Accessor> &accessors,
	           const std::vector<BufferView> &views, const std::vector<char> &binary,
	           const std::vector<engine::assets::GlbNode> &nodes,
	           std::vector<engine::assets::GlbSkin> &skins, std::string &error)
{
	for (const JsonValue &value : values)
	{
		const JsonObject *object = value.object();
		const JsonValue *jointsValue = object ? member(*object, "joints") : nullptr;
		std::size_t inverseBindAccessorIndex = 0;
		if (!object || !jointsValue || !jointsValue->array() ||
		    !readMemberIndex(*object, "inverseBindMatrices",
		                     inverseBindAccessorIndex) ||
		    inverseBindAccessorIndex >= accessors.size())
		{
			error = "skin must reference joints and inverse bind matrices";
			return false;
		}
		const JsonValue *nameValue = member(*object, "name");
		const std::string *name = nameValue ? nameValue->string() : nullptr;
		engine::assets::GlbSkin skin{.name = name ? *name : std::string{},
		                             .jointNodeIndices = {},
		                             .inverseBindMatrices = {}};
		for (const JsonValue &jointValue : *jointsValue->array())
		{
			std::size_t jointNodeIndex = 0;
			if (!readIndex(jointValue, jointNodeIndex) ||
			    jointNodeIndex >= nodes.size())
			{
				error = "skin joint must reference a valid node";
				return false;
			}
			skin.jointNodeIndices.push_back(jointNodeIndex);
		}
		const Accessor &inverseBindAccessor = accessors[inverseBindAccessorIndex];
		if (skin.jointNodeIndices.empty() ||
		    inverseBindAccessor.bufferViewIndex >= views.size() ||
		    inverseBindAccessor.componentType != 5126 ||
		    inverseBindAccessor.type != "MAT4" ||
		    inverseBindAccessor.count != skin.jointNodeIndices.size())
		{
			error = "inverse bind matrices must be FLOAT MAT4 values matching joints";
			return false;
		}
		const BufferView &inverseBindView = views[inverseBindAccessor.bufferViewIndex];
		skin.inverseBindMatrices.reserve(inverseBindAccessor.count);
		for (std::size_t matrixIndex = 0; matrixIndex < inverseBindAccessor.count;
		     ++matrixIndex)
		{
			std::size_t offset = 0;
			if (!elementOffset(inverseBindView, inverseBindAccessor,
			                   sizeof(float) * 16U, matrixIndex, binary.size(),
			                   offset))
			{
				error = "inverse bind matrix accessor exceeds its bufferView";
				return false;
			}
			engine::assets::GlbInverseBindMatrix matrix;
			std::memcpy(matrix.elements.data(), binary.data() + offset,
			            sizeof(float) * matrix.elements.size());
			for (float element : matrix.elements)
			{
				if (!std::isfinite(element))
				{
					error = "inverse bind matrix values must be finite";
					return false;
				}
			}
			skin.inverseBindMatrices.push_back(matrix);
		}
		skins.push_back(std::move(skin));
	}
	return true;
}

bool validateSkinnedNodes(const std::vector<engine::assets::GlbNode> &nodes,
	                      const std::vector<engine::assets::GlbMesh> &meshes,
	                      const std::vector<engine::assets::GlbSkin> &skins,
	                      std::string &error)
{
	for (const engine::assets::GlbNode &node : nodes)
	{
		if (!node.skinIndex)
		{
			continue;
		}
		if (*node.skinIndex >= skins.size() || !node.meshIndex ||
		    *node.meshIndex >= meshes.size())
		{
			error = "skinned node must reference a valid skin and mesh";
			return false;
		}
		const engine::assets::GlbSkin &skin = skins[*node.skinIndex];
		for (const engine::assets::GlbPrimitive &primitive :
		     meshes[*node.meshIndex].primitives)
		{
			if (primitive.source.jointIndices.size() !=
			        primitive.source.positions.size() ||
			    primitive.source.jointWeights.size() !=
			        primitive.source.positions.size())
			{
				error = "skinned primitive must provide joints and weights per vertex";
				return false;
			}
			for (const engine::assets::StaticMeshJointIndices &indices :
			     primitive.source.jointIndices)
			{
				if (indices.first >= skin.jointNodeIndices.size() ||
				    indices.second >= skin.jointNodeIndices.size() ||
				    indices.third >= skin.jointNodeIndices.size() ||
				    indices.fourth >= skin.jointNodeIndices.size())
				{
					error = "JOINTS_0 index exceeds the skin joint count";
					return false;
				}
			}
		}
	}
	return true;
}

bool readNodeTransformComponents(const JsonObject &node, const char *memberName,
	                             std::size_t expectedCount, float *components,
	                             std::string &error)
{
	const JsonValue *value = member(node, memberName);
	if (!value)
	{
		return true;
	}
	const JsonArray *values = value->array();
	if (!values || values->size() != expectedCount)
	{
		error = std::string("node ") + memberName +
		        " must contain the expected number of float values";
		return false;
	}
	for (std::size_t componentIndex = 0; componentIndex < expectedCount;
	     ++componentIndex)
	{
		const double *component = values->at(componentIndex).number();
		if (!component || !std::isfinite(*component))
		{
			error = std::string("node ") + memberName +
		        " values must be finite numbers";
			return false;
		}
		components[componentIndex] = static_cast<float>(*component);
	}
	return true;
}

bool readNodeHierarchy(const JsonArray &values,
	                   std::vector<engine::assets::GlbNode> &nodes,
	                   std::string &error)
{
	if (values.size() != nodes.size())
	{
		error = "node hierarchy count does not match nodes";
		return false;
	}
	for (std::size_t parentIndex = 0; parentIndex < values.size(); ++parentIndex)
	{
		const JsonObject *object = values[parentIndex].object();
		const JsonValue *childrenValue = object ? member(*object, "children") : nullptr;
		if (childrenValue && !childrenValue->array())
		{
			error = "node children must be an array";
			return false;
		}
		if (!childrenValue)
		{
			continue;
		}
		for (const JsonValue &childValue : *childrenValue->array())
		{
			std::size_t childIndex = 0;
			if (!readIndex(childValue, childIndex) || childIndex >= nodes.size())
			{
				error = "node child must reference a valid node";
				return false;
			}
			if (nodes[childIndex].parentNodeIndex)
			{
				error = "node child must have only one parent";
				return false;
			}
			nodes[childIndex].parentNodeIndex = parentIndex;
			nodes[parentIndex].childNodeIndices.push_back(childIndex);
		}
	}
	std::vector<std::uint8_t> visitState(nodes.size(), 0);
	auto visitNode = [&](auto &&self, std::size_t nodeIndex) -> bool
	{
		if (visitState[nodeIndex] == 1)
		{
			error = "node hierarchy contains a cycle";
			return false;
		}
		if (visitState[nodeIndex] == 2)
		{
			return true;
		}
		visitState[nodeIndex] = 1;
		for (std::size_t childIndex : nodes[nodeIndex].childNodeIndices)
		{
			if (!self(self, childIndex))
			{
				return false;
			}
		}
		visitState[nodeIndex] = 2;
		return true;
	};
	for (std::size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
	{
		if (!visitNode(visitNode, nodeIndex))
		{
			return false;
		}
	}
	return true;
}

bool documentFromJson(const std::string &json, const std::vector<char> &binary,
                      engine::assets::GlbDocument &document, std::string &error)
{
	JsonValue root;
	if (!JsonParser(json).parse(root, error))
	{
		return false;
	}
	const JsonObject *rootObject = root.object();
	if (!rootObject)
	{
		error = "glTF JSON root is not an object";
		return false;
	}
	const JsonValue *nodesValue = member(*rootObject, "nodes");
	const JsonValue *meshesValue = member(*rootObject, "meshes");
	const JsonValue *materialsValue = member(*rootObject, "materials");
	const JsonValue *scenesValue = member(*rootObject, "scenes");
	const JsonValue *bufferViewsValue = member(*rootObject, "bufferViews");
	const JsonValue *accessorsValue = member(*rootObject, "accessors");
	const JsonValue *imagesValue = member(*rootObject, "images");
	const JsonValue *texturesValue = member(*rootObject, "textures");
	const JsonValue *animationsValue = member(*rootObject, "animations");
	const JsonValue *skinsValue = member(*rootObject, "skins");
	if (!nodesValue || !meshesValue || !materialsValue || !scenesValue ||
	    !bufferViewsValue || !accessorsValue || !nodesValue->array() ||
	    !meshesValue->array() || !materialsValue->array() ||
	    !scenesValue->array() || !bufferViewsValue->array() ||
	    !accessorsValue->array())
	{
		error = "glTF JSON is missing required scene or geometry arrays";
		return false;
	}
	std::vector<BufferView> bufferViews;
	std::vector<Accessor> accessors;
	if (!readBufferViews(*bufferViewsValue->array(), bufferViews, error) ||
	    !readAccessors(*accessorsValue->array(), accessors, error))
	{
		return false;
	}
	if ((imagesValue && !imagesValue->array()) ||
	    (texturesValue && !texturesValue->array()) ||
	    (animationsValue && !animationsValue->array()) ||
	    (skinsValue && !skinsValue->array()))
	{
		error = "images, textures, animations and skins must be JSON arrays";
		return false;
	}
	if (imagesValue &&
	    !readImages(*imagesValue->array(), bufferViews, binary, document.images,
	                error))
	{
		return false;
	}
	if (texturesValue &&
	    !readTextures(*texturesValue->array(), document.images, document.textures,
	                  error))
	{
		return false;
	}

	for (const JsonValue &value : *materialsValue->array())
	{
		const JsonObject *object = value.object();
		if (!object)
		{
			error = "glTF material is not an object";
			return false;
		}
		const JsonValue *name = member(*object, "name");
		const std::string *text = name ? name->string() : nullptr;
		engine::assets::GlbMaterial material{
		    .name = text ? *text : std::string{},
		    .baseColor = {},
		    .emissiveColor = {},
		    .baseColorTexture = std::nullopt};
		if (!readBaseColor(*object, document.textures, material, error) ||
		    !readEmissiveColor(*object, material, error))
		{
			return false;
		}
		document.materials.push_back(std::move(material));
	}
	for (const JsonValue &value : *meshesValue->array())
	{
		const JsonObject *object = value.object();
		const JsonValue *name = object ? member(*object, "name") : nullptr;
		const JsonValue *primitives =
		    object ? member(*object, "primitives") : nullptr;
		if (!object || !primitives || !primitives->array())
		{
			error = "glTF mesh is missing its primitives array";
			return false;
		}
		const std::string *text = name ? name->string() : nullptr;
		engine::assets::GlbMesh mesh{.name = text ? *text : std::string{},
		                             .primitives = {}};
		for (const JsonValue &primitiveValue : *primitives->array())
		{
			const JsonObject *primitive = primitiveValue.object();
			const JsonValue *material =
			    primitive ? member(*primitive, "material") : nullptr;
			if (!primitive)
			{
				error = "glTF primitive is not an object";
				return false;
			}
			engine::assets::GlbPrimitive loadedPrimitive;
			std::optional<std::size_t> requiredTexCoord;
			if (!material)
			{
				loadedPrimitive.materialIndex = std::nullopt;
			}
			else
			{
				std::size_t materialIndex = 0;
				if (!readIndex(*material, materialIndex) ||
				    materialIndex >= document.materials.size())
				{
					error = "glTF primitive has an invalid material index";
					return false;
				}
				loadedPrimitive.materialIndex = materialIndex;
				if (document.materials[materialIndex].baseColorTexture)
				{
					requiredTexCoord =
					    document.materials[materialIndex].baseColorTexture->texCoord;
				}
			}
			// This index preserves the primitive-to-material association in CPU
			// data.
			if (!readStaticMeshSource(*primitive, accessors, bufferViews,
			                          binary, requiredTexCoord,
			                          loadedPrimitive.source, error))
			{
				return false;
			}
			mesh.primitives.push_back(std::move(loadedPrimitive));
		}
		document.meshes.push_back(std::move(mesh));
	}
	for (const JsonValue &value : *nodesValue->array())
	{
		const JsonObject *object = value.object();
		if (!object)
		{
			error = "glTF node is not an object";
			return false;
		}
		const JsonValue *name = member(*object, "name");
		const JsonValue *mesh = member(*object, "mesh");
		const JsonValue *skin = member(*object, "skin");
		if (member(*object, "matrix"))
		{
			error = "node matrix is not supported by the TRS CPU subset";
			return false;
		}
		const std::string *text = name ? name->string() : nullptr;
		engine::assets::GlbNode node{.name = text ? *text : std::string{},
		                             .meshIndex = std::nullopt,
		                             .skinIndex = std::nullopt,
		                             .parentNodeIndex = std::nullopt,
		                             .childNodeIndices = {},
		                             .localTransform = {}};
		float *translation = &node.localTransform.translationX;
		float *rotation = &node.localTransform.rotationX;
		float *scale = &node.localTransform.scaleX;
		if (!readNodeTransformComponents(*object, "translation", 3U,
		                                 translation, error) ||
		    !readNodeTransformComponents(*object, "rotation", 4U, rotation,
		                                 error) ||
		    !readNodeTransformComponents(*object, "scale", 3U, scale, error))
		{
			return false;
		}
		if (mesh)
		{
			std::size_t meshIndex = 0;
			if (!readIndex(*mesh, meshIndex) ||
			    meshIndex >= document.meshes.size())
			{
				error = "glTF node has an invalid mesh index";
				return false;
			}
			node.meshIndex = meshIndex;
		}
		if (skin)
		{
			std::size_t skinIndex = 0;
			if (!readIndex(*skin, skinIndex))
			{
				error = "glTF node has an invalid skin index";
				return false;
			}
			node.skinIndex = skinIndex;
		}
		document.nodes.push_back(std::move(node));
	}
	if (!readNodeHierarchy(*nodesValue->array(), document.nodes, error))
	{
		return false;
	}
	if (skinsValue &&
	    !readSkins(*skinsValue->array(), accessors, bufferViews, binary,
	               document.nodes, document.skins, error))
	{
		return false;
	}
	if (!validateSkinnedNodes(document.nodes, document.meshes, document.skins,
	                         error))
	{
		return false;
	}
	for (const JsonValue &value : *scenesValue->array())
	{
		const JsonObject *object = value.object();
		const JsonValue *nodes = object ? member(*object, "nodes") : nullptr;
		if (!object || !nodes || !nodes->array())
		{
			error = "glTF scene is missing its nodes array";
			return false;
		}
		const JsonValue *name = member(*object, "name");
		const std::string *text = name ? name->string() : nullptr;
		engine::assets::GlbScene scene{.name = text ? *text : std::string{},
		                               .nodeIndices = {}};
		for (const JsonValue &nodeValue : *nodes->array())
		{
			std::size_t nodeIndex = 0;
			if (!readIndex(nodeValue, nodeIndex) ||
			    nodeIndex >= document.nodes.size())
			{
				error = "glTF scene has an invalid node index";
				return false;
			}
			scene.nodeIndices.push_back(nodeIndex);
		}
		document.scenes.push_back(std::move(scene));
	}
	const JsonValue *activeScene = member(*rootObject, "scene");
	if (activeScene && (!readIndex(*activeScene, document.activeSceneIndex) ||
	                    document.activeSceneIndex >= document.scenes.size()))
	{
		error = "glTF has an invalid active scene index";
		return false;
	}
	if (animationsValue &&
	    !readAnimations(*animationsValue->array(), accessors, bufferViews, binary,
	                    document.nodes, document.animations, error))
	{
		return false;
	}
	return true;
}
} // namespace

engine::assets::GlbLoadResult::operator bool() const
{
	return document.has_value();
}

engine::assets::GlbLoadResult
engine::assets::loadGlb(const std::filesystem::path &path)
{
	std::vector<char> file;
	std::string error;
	if (!readBinaryFile(path, file, error))
	{
		return {.document = std::nullopt, .error = std::move(error)};
	}
	std::string json;
	std::vector<char> binary;
	if (!readGlbChunks(file, json, binary, error))
	{
		return {.document = std::nullopt, .error = std::move(error)};
	}
	GlbDocument document;
	if (!documentFromJson(json, binary, document, error))
	{
		return {.document = std::nullopt, .error = std::move(error)};
	}
	return {.document = std::move(document), .error = {}};
}
