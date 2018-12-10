open SwiftAst;

let join = (sep, nodes) =>
  switch (nodes) {
  | [] => []
  | [hd, ...tl] =>
    tl |> List.fold_left((acc, node) => acc @ [sep, node], [hd])
  };

let joinGroups = (sep, groups) => {
  let nonEmpty = groups |> List.filter(x => List.length(x) > 0);
  switch (nonEmpty) {
  | [] => []
  | [hd, ...tl] =>
    tl |> List.fold_left((acc, nodes) => acc @ [sep] @ nodes, hd)
  };
};

let nameWithoutExtension = path => {
  let obj: Node.Path.pathObject = Node.Path.parse(path);
  obj##name;
};

let nameWithoutPixelDensitySuffix = path =>
  Js.String.replaceByRe([%re "/@\\d+x$/g"], "", path);

let importFramework = framework =>
  switch (framework) {
  | SwiftOptions.UIKit => ImportDeclaration("UIKit")
  | SwiftOptions.AppKit => ImportDeclaration("AppKit")
  };

let colorTypeName = framework =>
  switch (framework) {
  | SwiftOptions.UIKit => "UIColor"
  | SwiftOptions.AppKit => "NSColor"
  };

let fontTypeName = framework =>
  switch (framework) {
  | SwiftOptions.UIKit => "UIFont"
  | SwiftOptions.AppKit => "NSFont"
  };

let imageTypeName = framework =>
  switch (framework) {
  | SwiftOptions.UIKit => "UIImage"
  | SwiftOptions.AppKit => "NSImage"
  };

let bezierPathTypeName = framework =>
  switch (framework) {
  | SwiftOptions.UIKit => "UIBezierPath"
  | SwiftOptions.AppKit => "NSBezierPath"
  };

let sizeTypeName = framework =>
  switch (framework) {
  | SwiftOptions.UIKit => "CGSize"
  | SwiftOptions.AppKit => "NSSize"
  };

let shadowTypeName = framework =>
  switch (framework) {
  | SwiftOptions.UIKit => "Shadow"
  | SwiftOptions.AppKit => "NSShadow"
  };

let layoutPriorityTypeDoc = framework =>
  switch (framework) {
  | SwiftOptions.UIKit => SwiftIdentifier("UILayoutPriority")
  | SwiftOptions.AppKit =>
    MemberExpression([
      SwiftIdentifier("NSLayoutConstraint"),
      SwiftIdentifier("Priority"),
    ])
  };

let labelAttributedTextName = framework =>
  switch (framework) {
  | SwiftOptions.UIKit => "attributedText"
  | SwiftOptions.AppKit => "attributedStringValue"
  };

let labelAttributedTextValue = framework =>
  switch (framework) {
  | SwiftOptions.UIKit =>
    labelAttributedTextName(framework) ++ " ?? NSAttributedString()"
  | SwiftOptions.AppKit => labelAttributedTextName(framework)
  };

let resizeModeValue = value =>
  switch (value) {
  | "cover" => "scaleAspectFill"
  | "contain" => "scaleAspectFit"
  | "stretch" => "scaleToFill"
  | _ =>
    Js.log("Invalid resizeMode");
    raise(Not_found);
  };

let localImageName = (framework: SwiftOptions.framework, name) => {
  let imageName =
    LiteralExpression(
      String(nameWithoutPixelDensitySuffix(nameWithoutExtension(name))),
    );
  switch (framework) {
  | SwiftOptions.UIKit => imageName
  | SwiftOptions.AppKit =>
    FunctionCallExpression({
      "name":
        MemberExpression([
          SwiftIdentifier("NSImage"),
          SwiftIdentifier("Name"),
        ]),
      "arguments": [
        FunctionCallArgument({
          "name": Some(SwiftIdentifier("rawValue")),
          "value": imageName,
        }),
      ],
    })
  };
};

let rec typeAnnotationDoc =
        (framework: SwiftOptions.framework, ltype: Types.lonaType) =>
  switch (ltype) {
  | Types.Reference(typeName) when LonaValue.isOptionalTypeName(typeName) =>
    let unwrapped = LonaValue.unwrapOptionalType(ltype);
    OptionalType(typeAnnotationDoc(framework, unwrapped));
  | Types.Reference(typeName) =>
    switch (typeName) {
    | "Boolean" => TypeName("Bool")
    | "Number" => TypeName("CGFloat")
    | "URL" =>
      typeAnnotationDoc(framework, Types.Named(typeName, Types.stringType))
    | "Color" =>
      typeAnnotationDoc(framework, Types.Named(typeName, Types.stringType))
    | _ => TypeName(typeName)
    }
  | Named("URL", _) => TypeName(imageTypeName(framework))
  | Named("Color", _) => TypeName(colorTypeName(framework))
  | Named(name, _) => TypeName(name)
  | Function(_, _) => TypeName("(() -> Void)?")
  | Array(elementType) =>
    ArrayType(typeAnnotationDoc(framework, elementType))
  | Variant(_) => TypeName("VARIANT PLACEHOLDER")
  };

let rec lonaValue =
        (
          framework: SwiftOptions.framework,
          config: Config.t,
          value: Types.lonaValue,
        ) =>
  switch (value.ltype) {
  | Reference(typeName) when LonaValue.isOptionalTypeName(typeName) =>
    switch (LonaValue.decodeOptional(value)) {
    | Some(innerValue) => lonaValue(framework, config, innerValue)
    | None => LiteralExpression(Nil)
    }
  | Reference(typeName) =>
    switch (typeName) {
    | "Boolean" => LiteralExpression(Boolean(value.data |> Json.Decode.bool))
    | "Number" =>
      LiteralExpression(FloatingPoint(value.data |> Json.Decode.float))
    | "String" => LiteralExpression(String(value.data |> Json.Decode.string))
    | "TextStyle"
    | "Color"
    | "Shadow" =>
      lonaValue(
        framework,
        config,
        {ltype: Named(typeName, Reference("String")), data: value.data},
      )
    | "URL" =>
      lonaValue(
        framework,
        config,
        {ltype: Named(typeName, Reference("String")), data: value.data},
      )
    | _ =>
      let match =
        UserTypes.find(config.userTypesFile.contents.types, typeName);
      switch (match) {
      | Some(Named(_, referencedType)) =>
        lonaValue(
          framework,
          config,
          {ltype: referencedType, data: value.data},
        )
      | Some(_) => SwiftIdentifier("UnknownNamedReferenceType: " ++ typeName)
      | None => SwiftIdentifier("UnknownReferenceType: " ++ typeName)
      };
    }
  | Variant(_) => SwiftIdentifier("." ++ (value.data |> Json.Decode.string))
  | Array(_) => SwiftIdentifier("PLACEHOLDER")
  | Function(_) => SwiftIdentifier("PLACEHOLDER")
  | Named(alias, subtype) =>
    switch (alias) {
    | "Color" =>
      let rawValue = value.data |> Json.Decode.string;
      switch (Color.find(config.colorsFile.contents, rawValue)) {
      | Some(color) =>
        MemberExpression([
          SwiftIdentifier("Colors"),
          SwiftIdentifier(color.id),
        ])
      | None => LiteralExpression(Color(rawValue))
      };
    | "URL" =>
      let rawValue = value.data |> Json.Decode.string;
      if (rawValue |> Js.String.startsWith("file://./")) {
        LiteralExpression
          (
            Image(
              nameWithoutPixelDensitySuffix(nameWithoutExtension(rawValue)),
            ),
          );
          /* FunctionCallExpression({
               "name": SwiftIdentifier(imageTypeName(framework)),
               "arguments": [
                 FunctionCallArgument({
                   "name": Some(SwiftIdentifier("named")),
                   "value": localImageName(framework, rawValue)
                 })
               ]
             }); */
      } else {
        Js.log2("WARNING: Image not handled", rawValue);
        Builders.functionCall([imageTypeName(framework)], []);
      };
    | "TextStyle" =>
      let rawValue = value.data |> Json.Decode.string;
      let textStyles = config.textStylesFile.contents;
      switch (TextStyle.find(textStyles.styles, rawValue)) {
      | Some(textStyle) =>
        MemberExpression([
          SwiftIdentifier("TextStyles"),
          SwiftIdentifier(textStyle.id),
        ])
      | None =>
        MemberExpression([
          SwiftIdentifier("TextStyles"),
          SwiftIdentifier(textStyles.defaultStyle.id),
        ])
      };
    | "Shadow" =>
      let rawValue = value.data |> Json.Decode.string;
      let shadows = config.shadowsFile.contents;
      switch (Shadow.find(shadows.styles, rawValue)) {
      | Some(shadow) =>
        MemberExpression([
          SwiftIdentifier("Shadows"),
          SwiftIdentifier(shadow.id),
        ])
      | None =>
        MemberExpression([
          SwiftIdentifier("Shadows"),
          SwiftIdentifier(shadows.defaultStyle.id),
        ])
      };
    | _ => lonaValue(framework, config, {ltype: subtype, data: value.data})
    }
  };

let rec defaultValueForLonaType =
        (
          framework: SwiftOptions.framework,
          config: Config.t,
          ltype: Types.lonaType,
        ) =>
  switch (ltype) {
  | Reference(typeName) when LonaValue.isOptionalTypeName(typeName) =>
    LiteralExpression(Nil)
  | Reference(typeName) =>
    switch (typeName) {
    | "Boolean" => LiteralExpression(Boolean(false))
    | "Number" => LiteralExpression(FloatingPoint(0.))
    | "String" => LiteralExpression(String(""))
    | "TextStyle"
    | "Color" =>
      defaultValueForLonaType(
        framework,
        config,
        Named(typeName, Reference("String")),
      )
    | "URL" =>
      defaultValueForLonaType(
        framework,
        config,
        Named(typeName, Reference("String")),
      )
    | _ =>
      let match =
        UserTypes.find(config.userTypesFile.contents.types, typeName);
      switch (match) {
      | Some(Named(_, referencedType)) =>
        defaultValueForLonaType(framework, config, referencedType)
      | Some(_) => LiteralExpression(Nil)
      | None => LiteralExpression(Nil)
      };
    }
  | Array(_) => LiteralExpression(Array([]))
  | Variant(cases) => SwiftIdentifier("." ++ List.nth(cases, 0))
  | Function(_) => SwiftIdentifier("PLACEHOLDER")
  | Named(alias, subtype) =>
    switch (alias) {
    | "Color" =>
      MemberExpression([
        SwiftIdentifier(colorTypeName(framework)),
        SwiftIdentifier("clear"),
      ])
    | "URL" =>
      FunctionCallExpression({
        "name": SwiftIdentifier(imageTypeName(framework)),
        "arguments": [],
      })
    | "TextStyle" =>
      MemberExpression([
        SwiftIdentifier("TextStyles"),
        SwiftIdentifier(config.textStylesFile.contents.defaultStyle.id),
      ])
    | _ => defaultValueForLonaType(framework, config, subtype)
    }
  };

let memberOrSelfExpression = (first, statements) =>
  switch (first) {
  | SwiftIdentifier("self") => MemberExpression(statements)
  | _ => MemberExpression([first] @ statements)
  };

let layerNameOrSelf = (rootLayer, layer: Types.layer) =>
  SwiftIdentifier(
    layer === rootLayer ? "self" : layer.name |> SwiftFormat.layerName,
  );

let layerMemberExpression = (rootLayer, layer: Types.layer, statements) =>
  memberOrSelfExpression(layerNameOrSelf(rootLayer, layer), statements);

let rec binaryExpressionList = (operator, items) =>
  switch (items) {
  | [] => Empty
  | [x] => x
  | [hd, ...tl] =>
    BinaryExpression({
      "left": hd,
      "operator": operator,
      "right": binaryExpressionList(operator, tl),
    })
  };