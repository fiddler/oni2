open EditorCoreTypes;
open Oni_Core;

type bufferPosition = {
  line: EditorCoreTypes.LineNumber.t,
  byteOffset: ByteIndex.t,
  characterOffset: CharacterIndex.t,
};

type t = {
  wrap: WordWrap.t,
  buffer: EditorBuffer.t,
  wraps: array(list(WordWrap.lineWrap)),
};

module Internal = {
  let bufferToWraps = (~wrap, buffer) => {
    let bufferLineCount = EditorBuffer.numberOfLines(buffer);
    let wraps = Array.make(bufferLineCount, []);

    for (idx in 0 to bufferLineCount - 1) {
      let line = EditorBuffer.line(idx, buffer);
      wraps[idx] = wrap(line);
    };
    wraps;
  };

  let bufferLineToViewLine = (bufferLine, {wraps, _}) => {
    let rec loop = (curr, idx) =>
      if (idx >= bufferLine) {
        curr;
      } else {
        loop(curr + List.length(wraps[idx]), idx + 1);
      };

    loop(0, 0);
  };

  let viewLineToBufferLine = (viewLine, {wraps, _}) => {
    let len = Array.length(wraps);

    let rec loop = (bufferLine, currentLine, currentWraps, lastBufferPosition) =>
      if (currentLine > viewLine) {
        lastBufferPosition;
      } else if (bufferLine >= len) {
        lastBufferPosition;
      } else {
        switch (currentWraps) {
        | [] =>
          loop(
            bufferLine + 1,
            currentLine,
            wraps[bufferLine + 1],
            lastBufferPosition,
          )
        | [hd, ...tail] =>
          loop(
            bufferLine,
            currentLine + 1,
            tail,
            {
              line: EditorCoreTypes.LineNumber.ofZeroBased(bufferLine),
              byteOffset: hd.byte,
              characterOffset: hd.character,
            },
          )
        };
      };

    loop(
      -1,
      0,
      [],
      {
        line: EditorCoreTypes.LineNumber.zero,
        byteOffset: ByteIndex.zero,
        characterOffset: CharacterIndex.zero,
      },
    );
  };
};

let make = (~wrap: Oni_Core.WordWrap.t, ~buffer) => {
  wrap,
  buffer,
  wraps: Internal.bufferToWraps(~wrap, buffer),
};

let update = (~update as _: Oni_Core.BufferUpdate.t, ~newBuffer, {wrap, _}) => {
  wrap,
  buffer: newBuffer,
  wraps: Internal.bufferToWraps(~wrap, newBuffer),
};

let bufferBytePositionToViewLine = (~bytePosition: BytePosition.t, wrap) => {
  let line = EditorCoreTypes.LineNumber.toZeroBased(bytePosition.line);
  let startViewLine = Internal.bufferLineToViewLine(line, wrap);
  let byteIndex = bytePosition.byte;

  let viewLines = wrap.wraps[line];

  let rec loop = (idx, viewLines: list(WordWrap.lineWrap)) => {
    switch (viewLines) {
    | [] => idx
    | [_] => idx
    | [hd, next, ...tail] =>
      if (ByteIndex.(byteIndex >= hd.byte && byteIndex < next.byte)) {
        idx;
      } else {
        loop(idx + 1, [next, ...tail]);
      }
    };
  };

  let offset = loop(0, viewLines);
  startViewLine + offset;
};

let viewLineToBufferPosition = (~line: int, wrapping) =>
  Internal.viewLineToBufferLine(line, wrapping);

let numberOfLines = wrapping => {
  let len = Array.length(wrapping.wraps);
  Internal.bufferLineToViewLine(len, wrapping);
};

let maxLineLength = ({buffer, _}) =>
  EditorBuffer.getEstimatedMaxLineLength(buffer);
