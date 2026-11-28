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

| Problem | Exit status |
|---|---|
| The file cannot be found or read | 2 |
| The content is invalid | 1 |
| No `language` is set (and no `--lang`) | 1 |
| The template or the language file is not found in any directory | 2 |
