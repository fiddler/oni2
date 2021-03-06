/*
 * CompletionMeet.re
 *
 * Helpers for finding the completion 'meet' (the location where we should request completions),
 * based on the current line and trigger characters
 */

open EditorCoreTypes;
open Oni_Core;

module Zed_utf8 = ZedBundled;

type t = {
  bufferId: int,
  // Base is the prefix string
  base: string,
  // Meet is the location where we request completions
  location: CharacterPosition.t,
};

let toString = (meet: t) =>
  Printf.sprintf(
    "Base: |%s| Meet: %s",
    meet.base,
    meet.location |> CharacterPosition.show,
  );

let defaultTriggerCharacters = [Uchar.of_char('.')];

let fromLine =
    (
      ~triggerCharacters=defaultTriggerCharacters,
      ~lineNumber=0,
      ~bufferId,
      ~index: CharacterIndex.t,
      line: BufferLine.t,
    ) => {
  let cursorIdx = CharacterIndex.toInt(index);
  let idx =
    Stdlib.min(
      BufferLine.lengthBounded(
        ~max=CharacterIndex.ofInt(cursorIdx + 1),
        line,
      )
      - 1,
      cursorIdx,
    );
  let pos = ref(idx);

  let matchesTriggerCharacters = c => {
    List.exists(tc => Uchar.equal(c, tc), triggerCharacters);
  };

  let lastCharacter = ref(None);
  let found = ref(false);

  let candidateBase = ref([]);

  while (pos^ >= 0 && ! found^) {
    let c = BufferLine.getUcharExn(~index=CharacterIndex.ofInt(pos^), line);
    lastCharacter := Some(c);

    if (matchesTriggerCharacters(c)
        || Uucp.White.is_white_space(c)
        && List.length(candidateBase^) > 0) {
      found := true;
      incr(pos);
    } else {
      candidateBase := [Zed_utf8.singleton(c), ...candidateBase^];
      decr(pos);
    };
  };

  let base = candidateBase^ |> String.concat("");

  let baseLength = Zed_utf8.length(base);

  switch (pos^) {
  | (-1) =>
    if (baseLength == cursorIdx && baseLength > 0) {
      Some({
        bufferId,
        location:
          CharacterPosition.{
            line: EditorCoreTypes.LineNumber.ofZeroBased(lineNumber),
            character: CharacterIndex.zero,
          },
        base,
      });
    } else {
      None;
    }
  | v =>
    Some({
      bufferId,
      location:
        CharacterPosition.{
          line: EditorCoreTypes.LineNumber.ofZeroBased(lineNumber),
          character: CharacterIndex.ofInt(v),
        },
      base,
    })
  };
};

let fromBufferPosition =
    (
      ~triggerCharacters=defaultTriggerCharacters,
      ~position: CharacterPosition.t,
      buffer: Buffer.t,
    ) => {
  let bufferLines = Buffer.getNumberOfLines(buffer);
  let line0 = EditorCoreTypes.LineNumber.toZeroBased(position.line);

  if (line0 < bufferLines) {
    let line = Buffer.getLine(line0, buffer);
    fromLine(
      ~bufferId=Buffer.getId(buffer),
      ~lineNumber=line0,
      ~triggerCharacters,
      ~index=position.character,
      line,
    );
  } else {
    None;
  };
};
