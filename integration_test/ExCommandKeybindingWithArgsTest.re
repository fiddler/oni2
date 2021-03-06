open Oni_Core;
open Oni_Model;
open Oni_IntegrationTestLib;
open Actions;

let keybindings =
  Some(
    {|
[
  {"key": "kk", "command": ":d 2", "when": "editorTextFocus"}
]
|},
  );

runTest(
  ~keybindings,
  ~name="ExCommandKeybindingTest",
  (dispatch, wait, _) => {
    let input = key => {
      let keyPress =
        EditorInput.KeyPress.{
          scancode: Sdl2.Scancode.ofName(key),
          keycode: Sdl2.Keycode.ofName(key),
          modifiers: EditorInput.Modifiers.none,
        };
      let time = Revery.Time.now();

      dispatch(KeyDown(keyPress, time));
      //dispatch(TextInput(key));
      dispatch(KeyUp(keyPress, time));
    };

    let testFile = getAssetPath("some-test-file.txt");
    dispatch(Actions.OpenFileByPath(testFile, None, None));

    wait(~name="Verify buffer is loaded", (state: State.t) =>
      switch (Selectors.getActiveBuffer(state)) {
      | Some(buffer) =>
        Buffer.getShortFriendlyName(buffer) == Some("some-test-file.txt")
      | None => false
      }
    );

    wait(~name="Verify initial line count", (state: State.t) =>
      switch (Selectors.getActiveBuffer(state)) {
      | Some(buffer) => Buffer.getNumberOfLines(buffer) == 3
      | None => false
      }
    );

    input("k");
    input("k");

    wait(~name="Wait for split to be created", (state: State.t) =>
      switch (Selectors.getActiveBuffer(state)) {
      | Some(buffer) => Buffer.getNumberOfLines(buffer) == 1
      | None => false
      }
    );
  },
);
