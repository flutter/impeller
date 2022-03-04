// ignore_for_file: camel_case_types
import 'dart:ffi' as ffi;
import 'dart:io';
import 'dart:typed_data';

/// Creates vertices from path commands.
///
/// First, build up the path contours with the [moveTo], [lineTo], [cubicTo],
/// and [close] methods. All methods expect absolute coordinates.
///
/// Then, use the [tessellate] method to create a [Float32List] of vertex pairs.
///
/// Finally, use the [dispose] method to clean up native resources. After
/// [dispose] has been called, this class must not be used again.
class VerticesBuilder {
  VerticesBuilder() : _builder = _createPathFn();

  ffi.Pointer<_PathBuilder>? _builder;
  final List<ffi.Pointer<_Vertices>> _vertices = <ffi.Pointer<_Vertices>>[];

  /// Adds a move verb to the absolute coordinates x,y.
  void moveTo(double x, double y) {
    assert(_builder != null);
    _moveToFn(_builder!, x, y);
  }

  /// Adds a line verb to the absolute coordinates x,y.
  void lineTo(double x, double y) {
    assert(_builder != null);
    _lineToFn(_builder!, x, y);
  }

  /// Adds a cubic Bezier curve with x1,y1 as the first control point, x2,y2 as
  /// the second control point, and end point x3,y3.
  void cubicTo(
    double x1,
    double y1,
    double x2,
    double y2,
    double x3,
    double y3,
  ) {
    assert(_builder != null);
    _cubicToFn(_builder!, x1, y1, x2, y2, x3, y3);
  }

  /// Adds a close command to the start of the current contour.
  void close() {
    assert(_builder != null);
    closeFn(_builder!, true);
  }

  /// Tessellates the path created by the previous method calls into a list of
  /// vertices.
  Float32List tessellate() {
    assert(_vertices.isEmpty);
    assert(_builder != null);
    final ffi.Pointer<_Vertices> vertices = _tessellateFn(_builder!);
    _vertices.add(vertices);
    return vertices.ref.points.asTypedList(vertices.ref.size);
  }

  /// Releases native resources.
  ///
  /// After calling dispose, this class must not be used again.
  void dispose() {
    assert(_builder != null);
    _vertices.forEach(_destroyVerticesFn);
    _destroyFn(_builder!);
    _vertices.clear();
    _builder = null;
  }
}

final ffi.DynamicLibrary _dylib = () {
  if (Platform.isWindows) {
    return ffi.DynamicLibrary.open('tessellator.dll');
  } else if (Platform.isIOS || Platform.isMacOS) {
    return ffi.DynamicLibrary.open('libtessellator.dylib');
  } else if (Platform.isAndroid || Platform.isLinux) {
    return ffi.DynamicLibrary.open('libtessellator.so');
  }
  throw UnsupportedError('Unknown platform: ${Platform.operatingSystem}');
}();

class _Vertices extends ffi.Struct {
  external ffi.Pointer<ffi.Float> points;

  @ffi.Uint32()
  external int size;
}

class _PathBuilder extends ffi.Opaque {}

typedef _CreatePathBuilderType = ffi.Pointer<_PathBuilder> Function();
typedef _create_path_builder_type = ffi.Pointer<_PathBuilder> Function();

final _CreatePathBuilderType _createPathFn =
    _dylib.lookupFunction<_create_path_builder_type, _CreatePathBuilderType>(
        'CreatePathBuilder');

typedef _MoveToType = void Function(ffi.Pointer<_PathBuilder>, double, double);
typedef _move_to_type = ffi.Void Function(
    ffi.Pointer<_PathBuilder>, ffi.Float, ffi.Float);

final _MoveToType _moveToFn =
    _dylib.lookupFunction<_move_to_type, _MoveToType>('MoveTo');

typedef _LineToType = void Function(ffi.Pointer<_PathBuilder>, double, double);
typedef _line_to_type = ffi.Void Function(
    ffi.Pointer<_PathBuilder>, ffi.Float, ffi.Float);

final _LineToType _lineToFn =
    _dylib.lookupFunction<_line_to_type, _LineToType>('LineTo');

typedef _CubicToType = void Function(
    ffi.Pointer<_PathBuilder>, double, double, double, double, double, double);
typedef _cubic_to_type = ffi.Void Function(ffi.Pointer<_PathBuilder>, ffi.Float,
    ffi.Float, ffi.Float, ffi.Float, ffi.Float, ffi.Float);

final _CubicToType _cubicToFn =
    _dylib.lookupFunction<_cubic_to_type, _CubicToType>('CubicTo');

typedef _CloseType = void Function(ffi.Pointer<_PathBuilder>, bool);
typedef _close_type = ffi.Void Function(ffi.Pointer<_PathBuilder>, ffi.Bool);

final _CloseType closeFn =
    _dylib.lookupFunction<_close_type, _CloseType>('Close');

typedef _TessellateType = ffi.Pointer<_Vertices> Function(
    ffi.Pointer<_PathBuilder>);
typedef _tessellate_type = ffi.Pointer<_Vertices> Function(
    ffi.Pointer<_PathBuilder>);

final _TessellateType _tessellateFn =
    _dylib.lookupFunction<_tessellate_type, _TessellateType>('Tessellate');

typedef _DestroyType = void Function(ffi.Pointer<_PathBuilder>);
typedef _destroy_type = ffi.Void Function(ffi.Pointer<_PathBuilder>);

final _DestroyType _destroyFn =
    _dylib.lookupFunction<_destroy_type, _DestroyType>('DestroyPathBuilder');

typedef _DestroyVerticesType = void Function(ffi.Pointer<_Vertices>);
typedef _destroy_vertices_type = ffi.Void Function(ffi.Pointer<_Vertices>);

final _DestroyVerticesType _destroyVerticesFn =
    _dylib.lookupFunction<_destroy_vertices_type, _DestroyVerticesType>(
        'DestroyVertices');
