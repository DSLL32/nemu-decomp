object FormEditor: TFormEditor
  Left = 192
  Top = 109
  Width = 656
  Height = 465
  Caption = 'Combine Editor'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  OnResize = FormResize
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object ListBoxCommands: TListBox
    Left = 0
    Top = 69
    Width = 406
    Height = 362
    Align = alClient
    ItemHeight = 13
    TabOrder = 0
    OnClick = ListBoxCommandsClick
  end
  object PanelTop: TPanel
    Left = 0
    Top = 0
    Width = 648
    Height = 69
    Align = alTop
    BevelInner = bvRaised
    BevelOuter = bvLowered
    TabOrder = 1
    object LabelColor: TLabel
      Left = 308
      Top = 10
      Width = 27
      Height = 13
      BiDiMode = bdLeftToRight
      Caption = 'Color:'
      ParentBiDiMode = False
    end
    object LabelAlpha: TLabel
      Left = 308
      Top = 30
      Width = 30
      Height = 13
      Caption = 'Alpha:'
    end
    object Label1: TLabel
      Left = 308
      Top = 50
      Width = 32
      Height = 13
      Caption = 'Extras:'
    end
    object ButtonNewCommand: TButton
      Left = 8
      Top = 8
      Width = 97
      Height = 25
      Caption = 'New Command'
      TabOrder = 0
      OnClick = ButtonNewCommandClick
    end
    object ButtonDeleteCommand: TButton
      Left = 8
      Top = 36
      Width = 97
      Height = 25
      Caption = 'Delete command'
      TabOrder = 1
      OnClick = ButtonDeleteCommandClick
    end
    object ButtonUp: TButton
      Left = 112
      Top = 8
      Width = 60
      Height = 25
      Caption = 'Up'
      TabOrder = 2
      OnClick = ButtonUpClick
    end
    object ButtonDown: TButton
      Left = 112
      Top = 36
      Width = 60
      Height = 25
      Caption = 'Down'
      TabOrder = 3
      OnClick = ButtonDownClick
    end
    object ButtonSave: TButton
      Left = 176
      Top = 8
      Width = 60
      Height = 25
      Caption = 'Save'
      TabOrder = 4
      OnClick = ButtonSaveClick
    end
    object ButtonLoad: TButton
      Left = 176
      Top = 36
      Width = 60
      Height = 25
      Caption = 'Load'
      TabOrder = 5
      OnClick = ButtonLoadClick
    end
    object EditColor: TEdit
      Left = 344
      Top = 6
      Width = 300
      Height = 21
      Color = clBtnFace
      ReadOnly = True
      TabOrder = 6
      Text = 'EditColor'
    end
    object EditAlpha: TEdit
      Left = 344
      Top = 26
      Width = 300
      Height = 21
      Color = clBtnFace
      ReadOnly = True
      TabOrder = 7
      Text = 'EditAlpha'
    end
    object EditExtras: TEdit
      Left = 343
      Top = 46
      Width = 300
      Height = 21
      Color = clBtnFace
      ReadOnly = True
      TabOrder = 8
      Text = 'EditAlpha'
    end
    object ButtonCopy: TButton
      Left = 240
      Top = 8
      Width = 60
      Height = 25
      Caption = 'Copy'
      TabOrder = 9
      OnClick = ButtonCopyClick
    end
    object ButtonPaste: TButton
      Left = 240
      Top = 36
      Width = 60
      Height = 25
      Caption = 'Paste'
      TabOrder = 10
      OnClick = ButtonPasteClick
    end
  end
  object GroupBoxEditor: TGroupBox
    Left = 406
    Top = 69
    Width = 242
    Height = 362
    Align = alRight
    Caption = 'Combine Editor'
    TabOrder = 2
    object ComboBoxCommand: TComboBox
      Left = 8
      Top = 20
      Width = 225
      Height = 21
      Style = csDropDownList
      ItemHeight = 13
      TabOrder = 0
      OnChange = ComboBoxCommandChange
      Items.Strings = (
        'NewPass'
        'SetColorSources'
        'SetAlphaSources'
        'AssignTexture'
        'SetTFactor'
        'TempRegOperation'
        'PrerenderTexture'
        'QueryDevice')
    end
    object Panel1: TPanel
      Left = 8
      Top = 44
      Width = 225
      Height = 225
      BevelInner = bvLowered
      BorderWidth = 3
      TabOrder = 1
      object NotebookCommand: TNotebook
        Left = 5
        Top = 5
        Width = 215
        Height = 215
        Align = alClient
        TabOrder = 0
        OnPageChanged = NotebookCommandPageChanged
        object TPage
          Left = 0
          Top = 0
          Caption = 'NewPass'
          object LabelSrcBlend: TLabel
            Left = 8
            Top = 12
            Width = 67
            Height = 13
            Caption = 'Source Blend:'
          end
          object LabelDstBlend: TLabel
            Left = 8
            Top = 36
            Width = 55
            Height = 13
            Caption = 'Dest Blend:'
          end
          object PanelConditional: TPanel
            Left = 8
            Top = 68
            Width = 201
            Height = 141
            BevelInner = bvRaised
            BevelOuter = bvLowered
            TabOrder = 2
            Visible = False
            object LabelConditional: TLabel
              Left = 8
              Top = 16
              Width = 94
              Height = 13
              Caption = 'Only process pass if'
            end
            object LabelConditionalConstant: TLabel
              Left = 64
              Top = 116
              Width = 45
              Height = 13
              Caption = 'Constant:'
            end
            object LabelConditionalReg1: TLabel
              Left = 8
              Top = 40
              Width = 32
              Height = 13
              Caption = 'Reg 1:'
            end
            object LabelConditionalReg2: TLabel
              Left = 8
              Top = 88
              Width = 32
              Height = 13
              Caption = 'Reg 2:'
            end
            object ComboBoxConditionalReg1: TComboBox
              Left = 64
              Top = 36
              Width = 129
              Height = 21
              Style = csDropDownList
              ItemHeight = 13
              ItemIndex = 0
              TabOrder = 0
              Text = '0: Prim Color'
              OnChange = ParamaterChanged
              Items.Strings = (
                '0: Prim Color'
                '1: Prim Alpha'
                '2: Env Color'
                '3: Env Alpha'
                '4: Prim LOD Frac'
                '5: Temp 1'
                '6: Temp 2'
                '7: Temp 3'
                '8: Temp 4'
                '9: Temp 5'
                'A: Temp 6')
            end
            object ComboBoxConditionalReg2: TComboBox
              Left = 64
              Top = 84
              Width = 129
              Height = 21
              Style = csDropDownList
              ItemHeight = 13
              ItemIndex = 0
              TabOrder = 1
              Text = '0: Constant'
              OnChange = ComboBoxConditionalReg2Change
              Items.Strings = (
                '0: Constant'
                '1: Prim Color'
                '2: Prim Alpha'
                '3: Env Color'
                '4: Env Alpha'
                '5: Prim LOD Frac'
                '6: Temp 1'
                '7: Temp 2'
                '8: Temp 3'
                '9: Temp 4'
                'A: Temp 5'
                'B: Temp 6')
            end
            object EditConditionalConstant: TEdit
              Left = 112
              Top = 112
              Width = 81
              Height = 21
              TabOrder = 2
              Text = 'FF000000'
              OnChange = ParamaterChanged
            end
            object ComboBoxCondition: TComboBox
              Left = 8
              Top = 60
              Width = 185
              Height = 21
              Style = csDropDownList
              ItemHeight = 13
              ItemIndex = 0
              TabOrder = 3
              Text = '== equals'
              OnChange = ParamaterChanged
              Items.Strings = (
                '== equals'
                '!= is not equal to'
                '> is greater than'
                '>= is greater than or equal'
                '< is smaller than'
                '<= is smaller than or equal')
            end
          end
          object CheckBoxConditional: TCheckBox
            Left = 16
            Top = 60
            Width = 81
            Height = 17
            Caption = 'Conditional'
            TabOrder = 3
            OnClick = CheckBoxConditionalClick
          end
          object ComboBoxSrcBlend: TComboBox
            Left = 80
            Top = 8
            Width = 129
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 0
            OnChange = ParamaterChanged
            Items.Strings = (
              '1: Zero'
              '2: One'
              '3: SourceColor'
              '4: 1-SourceColor'
              '5: SourceAlpha'
              '6: 1-SourceAlpha'
              '7: DestAlpha'
              '8: 1-DestAlpha'
              '9: DestColor'
              'A: 1-DestColor'
              'B: (SourceAlphaSat)'
              'C: (BothSourceAlpha)'
              'D: (BothInvSourceAlpha)')
          end
          object ComboBoxDstBlend: TComboBox
            Left = 80
            Top = 32
            Width = 129
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 1
            OnChange = ParamaterChanged
            Items.Strings = (
              '1: Zero'
              '2: One'
              '3: SourceColor'
              '4: 1-SourceColor'
              '5: SourceAlpha'
              '6: 1-SourceAlpha'
              '7: DestAlpha'
              '8: 1-DestAlpha'
              '9: DestColor'
              'A: 1-DestColor'
              'B: (SourceAlphaSat)'
              'C: (BothSourceAlpha)'
              'D: (BothInvSourceAlpha)')
          end
        end
        object TPage
          Left = 0
          Top = 0
          Caption = 'Set Color/Alpha Sources'
          object LabelOperation: TLabel
            Left = 83
            Top = 12
            Width = 49
            Height = 13
            Caption = 'Operation:'
          end
          object LabelSource1: TLabel
            Left = 83
            Top = 60
            Width = 46
            Height = 13
            Caption = 'Source 1:'
          end
          object LabelSource2: TLabel
            Left = 83
            Top = 108
            Width = 46
            Height = 13
            Caption = 'Source 2:'
          end
          object LabelSource3: TLabel
            Left = 83
            Top = 156
            Width = 46
            Height = 13
            Caption = 'Source 0:'
          end
          object ComboBoxOperation: TComboBox
            Left = 15
            Top = 28
            Width = 185
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 0
            OnChange = ComboBoxOperationChange
            Items.Strings = (
              '1: Disable'
              '2: Select Arg 1'
              '3: Select Arg 2'
              '4: Modulate'
              '5: Modulate 2x'
              '6: Modulate 4x'
              '7: Add'
              '8: Add Signed'
              '9: Add Signed 2x'
              'A: Subtract'
              'B: Add Smooth'
              'C: Blend Diffuse Alpha'
              'D: Blend Texture Alpha'
              'E: Blend Factor Alpha'
              'F: Blend Texture Alpha PM'
              '10: Blend Current Alpha'
              '11: Premodulate'
              '12: Modulate Alpha Add Color'
              '13: Modulate Color Add Alpha'
              '14: Modulate Inv Alpha Add Color'
              '15: Modulate Inv Color Add Alpha'
              '16: Bump Env Map'
              '17: Bump Env Map Luminance'
              '18: Dot Product 3'
              '19: Multiply Add'
              '1A: Lerp'
              ' ')
          end
          object ComboBoxSource1: TComboBox
            Left = 15
            Top = 76
            Width = 185
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 1
            OnChange = ParamaterChanged
            Items.Strings = (
              '0: Diffuse'
              '1: Current'
              '2: Texture'
              '3: TFactor'
              '4: (Specular)'
              '5: (Temp)'
              '10: 1-Diffuse'
              '11: 1-Current'
              '12: 1-Texture'
              '13: 1-TFactor'
              '14: (1-Specular)'
              '15: (1-Temp)'
              '20: AlphaReplicate - Diffuse'
              '21: AlphaReplicate - Current'
              '22: AlphaReplicate - Texture'
              '23: AlphaReplicate - TFactor'
              '24: AlphaReplicate - (Specular)'
              '25: AlphaReplicate - (Temp)'
              '30: AlphaReplicate - 1-Diffuse'
              '31: AlphaReplicate - 1-Current'
              '32: AlphaReplicate - 1-Texture'
              '33: AlphaReplicate - 1-TFactor'
              '34: AlphaReplicate - (1-Specular)'
              '35: AlphaReplicate - (1-Temp)')
          end
          object ComboBoxSource2: TComboBox
            Left = 15
            Top = 124
            Width = 185
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 2
            OnChange = ParamaterChanged
            Items.Strings = (
              '0: Diffuse'
              '1: Current'
              '2: Texture'
              '3: TFactor'
              '4: (Specular)'
              '5: (Temp)'
              '10: 1-Diffuse'
              '11: 1-Current'
              '12: 1-Texture'
              '13: 1-TFactor'
              '14: (1-Specular)'
              '15: (1-Temp)'
              '20: AlphaReplicate - Diffuse'
              '21: AlphaReplicate - Current'
              '22: AlphaReplicate - Texture'
              '23: AlphaReplicate - TFactor'
              '24: AlphaReplicate - (Specular)'
              '25: AlphaReplicate - (Temp)'
              '30: AlphaReplicate - 1-Diffuse'
              '31: AlphaReplicate - 1-Current'
              '32: AlphaReplicate - 1-Texture'
              '33: AlphaReplicate - 1-TFactor'
              '34: AlphaReplicate - (1-Specular)'
              '35: AlphaReplicate - (1-Temp)')
          end
          object ComboBoxSource3: TComboBox
            Left = 15
            Top = 172
            Width = 185
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 3
            OnChange = ParamaterChanged
            Items.Strings = (
              '0: Diffuse'
              '1: Current'
              '2: Texture'
              '3: TFactor'
              '4: (Specular)'
              '5: (Temp)'
              '10: 1-Diffuse'
              '11: 1-Current'
              '12: 1-Texture'
              '13: 1-TFactor'
              '14: (1-Specular)'
              '15: (1-Temp)'
              '20: AlphaReplicate - Diffuse'
              '21: AlphaReplicate - Current'
              '22: AlphaReplicate - Texture'
              '23: AlphaReplicate - TFactor'
              '24: AlphaReplicate - (Specular)'
              '25: AlphaReplicate - (Temp)'
              '30: AlphaReplicate - 1-Diffuse'
              '31: AlphaReplicate - 1-Current'
              '32: AlphaReplicate - 1-Texture'
              '33: AlphaReplicate - 1-TFactor'
              '34: AlphaReplicate - (1-Specular)'
              '35: AlphaReplicate - (1-Temp)')
          end
        end
        object TPage
          Left = 0
          Top = 0
          Caption = 'AssignTexture'
          object LabelAssignTextureStage: TLabel
            Left = 12
            Top = 12
            Width = 31
            Height = 13
            Caption = 'Stage:'
          end
          object RadioGroupTextureSource: TRadioGroup
            Left = 8
            Top = 32
            Width = 197
            Height = 177
            Caption = 'Source'
            ItemIndex = 0
            Items.Strings = (
              'Texture 0'
              'Texture 1'
              'Prerendered Texture 0'
              'Prerendered Texture 1'
              'Color Texture Prim Color'
              'Color Texture Env Color'
              'Color Texture Temp1'
              'Color Texture Temp2'
              'Color Texture Temp3'
              'Color Texture Temp4'
              'Color Texture Temp5'
              'Color Texture Temp6')
            TabOrder = 0
            OnClick = ParamaterChanged
          end
          object ComboBoxTextureStage: TComboBox
            Left = 48
            Top = 8
            Width = 41
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 1
            OnChange = ParamaterChanged
            Items.Strings = (
              '0'
              '1'
              '2'
              '3'
              '4'
              '5'
              '6'
              '7')
          end
        end
        object TPage
          Left = 0
          Top = 0
          Caption = 'SetTFactor'
          object RadioGroupTFactorColor: TRadioGroup
            Left = 4
            Top = 4
            Width = 97
            Height = 177
            Caption = 'Color'
            ItemIndex = 0
            Items.Strings = (
              'Prim Color'
              'Env Color'
              'PrimLODFrac'
              'Temp1'
              'Temp2'
              'Temp3'
              'Temp4'
              'Temp5'
              'Temp6'
              'Prim Alpha'
              'Env Alpha')
            TabOrder = 0
            OnClick = ParamaterChanged
          end
          object RadioGroupTFactorAlpha: TRadioGroup
            Left = 112
            Top = 4
            Width = 93
            Height = 149
            Caption = 'Alpha'
            ItemIndex = 0
            Items.Strings = (
              'Prim Alpha'
              'Env Alpha'
              'PrimLODFrac'
              'Temp1'
              'Temp2'
              'Temp3'
              'Temp4'
              'Temp5'
              'Temp6')
            TabOrder = 1
            OnClick = ParamaterChanged
          end
        end
        object TPage
          Left = 0
          Top = 0
          Caption = 'TempRegOp'
          object LabelTempRegOpSource1: TLabel
            Left = 8
            Top = 40
            Width = 69
            Height = 13
            Caption = 'Source Reg 1:'
          end
          object LabelTempRegOpTarget: TLabel
            Left = 8
            Top = 132
            Width = 57
            Height = 13
            Caption = 'Target Reg:'
          end
          object LabelTempRegOpOperation: TLabel
            Left = 8
            Top = 12
            Width = 49
            Height = 13
            Caption = 'Operation:'
          end
          object LabelTempRegOpSource2: TLabel
            Left = 8
            Top = 68
            Width = 69
            Height = 13
            Caption = 'Source Reg 2:'
          end
          object LabelTempRegOpImm: TLabel
            Left = 76
            Top = 96
            Width = 45
            Height = 13
            Caption = 'Constant:'
          end
          object ComboBoxTempRegOpSource1: TComboBox
            Left = 80
            Top = 36
            Width = 129
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 0
            OnChange = ParamaterChanged
            Items.Strings = (
              '0: Prim Color'
              '1: Prim Alpha'
              '2: Env Color'
              '3: Env Alpha'
              '4: Prim LOD Frac'
              '5: Temp 1'
              '6: Temp 2'
              '7: Temp 3'
              '8: Temp 4'
              '9: Temp 5'
              'A: Temp 6')
          end
          object ComboBoxTempRegOpTarget: TComboBox
            Left = 80
            Top = 128
            Width = 129
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 1
            OnChange = ParamaterChanged
            Items.Strings = (
              '0: Temp 1'
              '1: Temp 2'
              '2: Temp 3'
              '3: Temp 4'
              '4: Temp 5'
              '5: Temp 6')
          end
          object ComboBoxTempRegOpOperation: TComboBox
            Left = 64
            Top = 8
            Width = 145
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 2
            OnChange = ComboBoxTempRegOpOperationChange
            Items.Strings = (
              '0: Set'
              '1: Replicate Alpha'
              '2: Logical AND'
              '3: Logical OR'
              '4: Add'
              '5: Subtract'
              '6: Modulate')
          end
          object ComboBoxTempRegOpSource2: TComboBox
            Left = 80
            Top = 64
            Width = 129
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 3
            OnChange = ComboBoxTempRegOpSource2Change
            Items.Strings = (
              '0: Constant'
              '1: Prim Color'
              '2: Prim Alpha'
              '3: Env Color'
              '4: Env Alpha'
              '5: Prim LOD Frac'
              '6: Temp 1'
              '7: Temp 2'
              '8: Temp 3'
              '9: Temp 4'
              'A: Temp 5'
              'B: Temp 6')
          end
          object EditTempRegOpImm: TEdit
            Left = 124
            Top = 92
            Width = 81
            Height = 21
            TabOrder = 4
            Text = 'FF000000'
            OnChange = ParamaterChanged
          end
        end
        object TPage
          Left = 0
          Top = 0
          Caption = 'PrerenderTexture'
          object LabelSource: TLabel
            Left = 8
            Top = 12
            Width = 37
            Height = 13
            Caption = 'Source:'
          end
          object LabelPrerenderer: TLabel
            Left = 8
            Top = 40
            Width = 58
            Height = 13
            Caption = 'Prerenderer:'
          end
          object ComboBoxPrerenderTexture: TComboBox
            Left = 80
            Top = 8
            Width = 129
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 0
            OnClick = ParamaterChanged
            Items.Strings = (
              'Texture 0'
              'Texture 1')
          end
          object ComboBoxPrerenderer: TComboBox
            Left = 80
            Top = 36
            Width = 129
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            TabOrder = 1
            OnClick = ParamaterChanged
            Items.Strings = (
              '2ColTex (2 arg)'
              'Modulate (1 arg)'
              'ModulateAdd (2 arg)'
              'ModulateSub (2 arg)'
              'Sub (1 arg)'
              'ModulateColorReplaceAlpha (2 arg)'
              'ModulateAlphaReplaceColor (2 arg)'
              'ModulateOneMinusTexColor (1 arg)')
          end
          object RadioGroupPrerenderArg1: TRadioGroup
            Left = 8
            Top = 68
            Width = 97
            Height = 133
            Caption = 'Argument 1'
            ItemIndex = 0
            Items.Strings = (
              'Prim Color'
              'Env Color'
              'Temp 1'
              'Temp 2'
              'Temp 3'
              'Temp 4'
              'Temp 5'
              'Temp 6')
            TabOrder = 2
            OnClick = ParamaterChanged
          end
          object RadioGroupPrerenderArg2: TRadioGroup
            Left = 108
            Top = 68
            Width = 97
            Height = 133
            Caption = 'Argument 2'
            ItemIndex = 0
            Items.Strings = (
              'Prim Color'
              'Env Color'
              'Temp 1'
              'Temp 2'
              'Temp 3'
              'Temp 4'
              'Temp 5'
              'Temp 6')
            TabOrder = 3
            OnClick = ParamaterChanged
          end
        end
        object TPage
          Left = 0
          Top = 0
          Caption = 'QueryDevice'
          object LabelRead: TLabel
            Left = 8
            Top = 12
            Width = 29
            Height = 13
            Caption = 'Read:'
          end
          object LabelQueryTargetReg: TLabel
            Left = 8
            Top = 64
            Width = 57
            Height = 13
            Caption = 'Target Reg:'
          end
          object ComboBoxQueryDevice: TComboBox
            Left = 8
            Top = 32
            Width = 201
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 0
            Text = 'MaxSimultaneousTextures'
            OnChange = ParamaterChanged
            Items.Strings = (
              'MaxSimultaneousTextures'
              'MaxTextureBlendStages'
              'TEXOPCAPS_ADD'
              'TEXOPCAPS_ADDSIGNED'
              'TEXOPCAPS_ADDSIGNED2X'
              'TEXOPCAPS_ADDSMOOTH'
              'TEXOPCAPS_BLENDCURRENTALPHA'
              'TEXOPCAPS_BLENDDIFFUSEALPHA'
              'TEXOPCAPS_BLENDFACTORALPHA'
              'TEXOPCAPS_BLENDTEXTUREALPHA'
              'TEXOPCAPS_BLENDTEXTUREALPHAPM'
              'TEXOPCAPS_BUMPENVMAP'
              'TEXOPCAPS_BUMPENVMAPLUMINANCE'
              'TEXOPCAPS_DISABLE'
              'TEXOPCAPS_DOTPRODUCT3'
              'TEXOPCAPS_LERP'
              'TEXOPCAPS_MODULATE'
              'TEXOPCAPS_MODULATE2X'
              'TEXOPCAPS_MODULATE4X'
              'TEXOPCAPS_MODULATEALPHA_ADDCOLOR'
              'TEXOPCAPS_MODULATECOLOR_ADDALPHA'
              'TEXOPCAPS_MODULATEINVALPHA_ADDCOLOR'
              'TEXOPCAPS_MODULATEINVCOLOR_ADDALPHA'
              'TEXOPCAPS_MULTIPLYADD'
              'TEXOPCAPS_PREMODULATE'
              'TEXOPCAPS_SELECTARG1'
              'TEXOPCAPS_SELECTARG2'
              'TEXOPCAPS_SUBTRACT'
              ''
              'SRC: BLENDCAPS_BOTHINVSRCALPHA'
              'SRC: BLENDCAPS_BOTHSRCALPHA'
              'SRC: BLENDCAPS_DESTALPHA'
              'SRC: BLENDCAPS_DESTCOLOR'
              'SRC: BLENDCAPS_INVDESTALPHA'
              'SRC: BLENDCAPS_INVDESTCOLOR'
              'SRC: BLENDCAPS_INVSRCALPHA'
              'SRC: BLENDCAPS_INVSRCCOLOR'
              'SRC: BLENDCAPS_ONE'
              'SRC: BLENDCAPS_SRCALPHA'
              'SRC: BLENDCAPS_SRCALPHASAT'
              'SRC: BLENDCAPS_SRCCOLOR'
              'SRC: BLENDCAPS_ZERO'
              ''
              'DST: BLENDCAPS_BOTHINVSRCALPHA'
              'DST: BLENDCAPS_BOTHSRCALPHA'
              'DST: BLENDCAPS_DESTALPHA'
              'DST: BLENDCAPS_DESTCOLOR'
              'DST: BLENDCAPS_INVDESTALPHA'
              'DST: BLENDCAPS_INVDESTCOLOR'
              'DST: BLENDCAPS_INVSRCALPHA'
              'DST: BLENDCAPS_INVSRCCOLOR'
              'DST: BLENDCAPS_ONE'
              'DST: BLENDCAPS_SRCALPHA'
              'DST: BLENDCAPS_SRCALPHASAT'
              'DST: BLENDCAPS_SRCCOLOR'
              'DST: BLENDCAPS_ZERO'
              '')
          end
          object ComboBoxQueryTargetReg: TComboBox
            Left = 80
            Top = 60
            Width = 129
            Height = 21
            Style = csDropDownList
            ItemHeight = 13
            ItemIndex = 0
            TabOrder = 1
            Text = '0: Temp 1'
            OnChange = ParamaterChanged
            Items.Strings = (
              '0: Temp 1'
              '1: Temp 2'
              '2: Temp 3'
              '3: Temp 4'
              '4: Temp 5'
              '5: Temp 6')
          end
        end
      end
    end
  end
end
