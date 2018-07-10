let load_plug = fname => {
  let fname = Dynlink.adapt_filename(fname);
  if (Sys.file_exists(fname)) {
    try (Dynlink.loadfile(fname)) {
    | Dynlink.Error(err) as e =>
      print_endline("ERROR loading plugin: " ++ Dynlink.error_message(err));
    | _ => print_endline("Unknown error while loading plugin")
    };
  } else {
    print_endline("Plugin file does not exist");
  };
};

let last_st_mtime = ref(0.);

let extension = Sys.win32 || Sys.cygwin ? ".exe" : "";

let ocaml = (Dynlink.is_native ? "ocamlopt.opt" : "ocamlc.opt") ++ extension;

let extension = Dynlink.is_native ? "cmxs" : "cmo";

let shared = Dynlink.is_native ? "-shared" : "-c";

let folder = Dynlink.is_native ? "native" : "bytecode";

let (+/) = Filename.concat;

let libFilePath = "lib" +/ "bs" +/ "bytecode" +/ "lib.cma";

let bsb = "node_modules" +/ ".bin" +/ "bsb";

let checkRebuild = (firstTime, filePath) => {
  if (firstTime) {
    /* Compile once synchronously because we're going to load it immediately after this. */
    switch (
      Unix.system(
        bsb
        ++ " -build-library "
        ++ String.capitalize(
             Filename.chop_extension(Filename.basename(filePath)),
           ),
      )
    ) {
    | WEXITED(0) => ()
    | WEXITED(_)
    | WSIGNALED(_)
    | WSTOPPED(_) => print_endline("Hotreload failed")
    };
    let pid =
      Unix.create_process(
        bsb,
        [|
          bsb,
          "-w",
          "-build-library",
          String.capitalize(
            Filename.chop_extension(Filename.basename(filePath)),
          ),
        |],
        Unix.stdin,
        Unix.stdout,
        Unix.stderr,
      );
    print_endline("bsb running with pid: " ++ string_of_int(pid));
    /* 9 is SIGKILL */
    at_exit(() => Unix.kill(pid, 9));
    ();
  };
  if (Sys.file_exists(libFilePath)) {
    let {Unix.st_mtime} = Unix.stat(libFilePath);
    if (st_mtime > last_st_mtime^) {
      print_endline("Reloading hotloaded module");
      load_plug(libFilePath);
      last_st_mtime := st_mtime;
      true;
    } else {
      false;
    };
  } else {
    false;
  };
};
