# Configuration

byzantine-cal reads a configuration file when it generates a calendar (`--year`). It tells the
program where to find templates and language files, and which language to use.

## The configuration file

A YAML file with these options. All of them are optional, but a language and the directories that
hold its language file and the template are needed to generate a calendar.

```yaml
template_dir:          # directories to look for templates, in this order
  - /usr/share/tpl     # an absolute name, or
  - tpl                # a name relative to the directory of this file
lang_dir: [lang]       # directories to look for language files, the same way
language: de_DE        # the language: the file de_DE.yaml in a lang_dir
```

| Option | Meaning |
|---|---|
| `template_dir` | A list of directories to look for templates, see [Templates](templates.md). |
| `lang_dir` | A list of directories to look for language files, see [Languages](language.md). |
| `language` | The language to use. `--lang=<language>` overrides it. |

Names in `template_dir` and `lang_dir` are either absolute, or relative to the directory of the
configuration file, as it was given (a symlink is not followed). The first directory that has the
file wins.

Unknown options, an option that is set twice, a value of the wrong type and a file with several
YAML documents are errors. They are reported with the file and the line.

## Which file is used

In this order:

1. The file given with `-C <file>`. It is used as it is, so a missing file is an error and there is
   no fallback to the next place.
2. `bcal.yaml` in the current directory. If there is something with that name it is used, and
   reported if it cannot be read (a directory, a broken symlink).
3. `config.yaml` in the installation directory, `<prefix>/lib/byzantine-cal/config.yaml`. It is
   `/usr/local/lib/byzantine-cal/config.yaml` unless the prefix was set when building, see the
   [README](../README.md#install).

## The installed configuration

`config/config.yaml` is installed next to the templates and the language files, so an installed
program works without any configuration of its own:

```yaml
template_dir:
  - tpl
lang_dir:
  - lang
language: en_US
```

To use other templates or languages, write a `bcal.yaml` in the directory you work in, or give a
file with `-C`, for example one that lists your own directory first:

```yaml
template_dir:
  - my-templates
  - /usr/local/lib/byzantine-cal/tpl
lang_dir:
  - my-languages
  - /usr/local/lib/byzantine-cal/lang
language: de_CH
```

## Errors

A configuration file that cannot be read, a template or language file that is not found, and an
invalid configuration are errors, see the exit status in the [README](../README.md#run). So is a
missing `language`, when neither the configuration nor `--lang` sets one.

## Running without installing

The installed configuration names `tpl` and `lang` next to itself, which only exists after
installing. To run the program from the build directory, give it a configuration that points at the
sources, with `-C` or as `bcal.yaml`:

```yaml
template_dir: [/path/to/byzantine-cal/tpl]
lang_dir: [/path/to/byzantine-cal/lang]
language: en_US
```

## From C++

`ByzantineCalConfig` (`src/byzantine_cal_config.h`) reads the file. `find_file()` chooses the file as
described above, and the constructor reads it. `find_template()` and `find_language_file()` return
the path of a file, ready to use, or throw `ConfigFileError`. `ConfigFileError` is a `ConfigError`,
and the messages of both name the file and the line.

```cpp
auto file = bcal::ByzantineCalConfig::find_file(path_from_option_C, installed_config);
bcal::ByzantineCalConfig config(file);
auto template_path = config.find_template("markdown.inja");
```
