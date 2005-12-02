/*
 * 
 * 
 * Copyright (C) 2004 SIPfoundry Inc.
 * Licensed by SIPfoundry under the LGPL license.
 * 
 * Copyright (C) 2004 Pingtel Corp.
 * Licensed to SIPfoundry under a Contributor Agreement.
 * 
 * $
 */
package org.sipfoundry.sipxconfig.setting;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.Map;

import org.apache.commons.digester.BeanPropertySetterRule;
import org.apache.commons.digester.Digester;
import org.apache.commons.digester.Rule;
import org.apache.commons.digester.RuleSetBase;
import org.apache.commons.io.IOUtils;
import org.apache.commons.lang.StringUtils;
import org.sipfoundry.sipxconfig.setting.type.BooleanSetting;
import org.sipfoundry.sipxconfig.setting.type.EnumSetting;
import org.sipfoundry.sipxconfig.setting.type.FileSetting;
import org.sipfoundry.sipxconfig.setting.type.IntegerSetting;
import org.sipfoundry.sipxconfig.setting.type.RealSetting;
import org.sipfoundry.sipxconfig.setting.type.SettingType;
import org.sipfoundry.sipxconfig.setting.type.StringSetting;
import org.xml.sax.Attributes;
import org.xml.sax.EntityResolver;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

/**
 * Build a SettingModel object hierarchy from a model XML file.
 */
public class XmlModelBuilder implements ModelBuilder {
    private static final String EL_VALUE = "/value";
    private static final String EL_LABEL = "/label";

    private final Map m_types = new HashMap();
    private final File m_configDirectory;

    public XmlModelBuilder(File configDirectory) {
        m_configDirectory = configDirectory;
    }

    public XmlModelBuilder(String configDirectory) {
        this(new File(configDirectory));
    }
    
    public SettingSet buildModel(File modelFile) {
        return buildModel(modelFile, null);
    }
    
    public SettingSet buildModel(File modelFile, Setting parent) {
        FileInputStream is = null;
        try {
            is = new FileInputStream(modelFile);
            return buildModel(is, parent, modelFile.getParentFile());

        } catch (IOException e) {
            throw new RuntimeException("Cannot parse model definitions file "
                    + modelFile.getPath(), e);
        } finally {
            IOUtils.closeQuietly(is);
        }
    }

    public SettingSet buildModel(InputStream is, Setting parent) throws IOException {
        return buildModel(is, parent, null);
    }
    /*
     * (non-Javadoc)
     * 
     * @see org.sipfoundry.sipxconfig.setting.ModelBuilder#buildModel(java.io.InputStream)
     */
    public SettingSet buildModel(InputStream is, Setting parent, File baseSystemId) throws IOException {
        Digester digester = new Digester();

        // setting classloader ensures classes are searched for in this classloader
        // instead of parent's classloader is digister was loaded there.
        digester.setClassLoader(this.getClass().getClassLoader());
        digester.setValidating(false);
        EntityResolver entityResolver = new ModelEntityResolver(m_configDirectory, baseSystemId);
        digester.setEntityResolver(entityResolver);
        if (parent != null) {
            digester.push(parent.copy());
        } else {
            digester.push(new ConditionalSet());
        }
        addSettingTypes(digester, "model/type/");

        String groupPattern = "*/group";
        SettingRuleSet groupRule = new SettingRuleSet(groupPattern, ConditionalSet.class);
        digester.addRuleSet(groupRule);

        String settingPattern = "*/setting";
        SettingRuleSet settingRule = new SettingRuleSet(settingPattern, ConditionalSettingImpl.class);
        digester.addRuleSet(settingRule);

        try {
            return (SettingSet) digester.parse(is);
        } catch (SAXException se) {
            throw new RuntimeException("Could not parse model definition file", se);
        }
    }

    private void addSettingTypes(Digester digester, String patternPrefix) {
        digester.addRuleSet(new IntegerSettingRule(patternPrefix + "integer"));
        digester.addRuleSet(new RealSettingRule(patternPrefix + "real"));
        digester.addRuleSet(new StringSettingRule(patternPrefix + "string"));
        digester.addRuleSet(new EnumSettingRule(patternPrefix + "enum"));
        digester.addRuleSet(new BooleanSettingRule(patternPrefix + "boolean"));
        digester.addRuleSet(new FileSettingRule(patternPrefix + "file"));
    }

    class SettingRuleSet extends RuleSetBase {

        private String m_pattern;

        private Class m_class;

        public SettingRuleSet(String pattern, Class c) {
            m_pattern = pattern;
            m_class = c;
        }

        public void addRuleInstances(Digester digester) {
            digester.addObjectCreate(m_pattern, m_class);
            digester.addSetProperties(m_pattern, "parent", null);
            digester.addRule(m_pattern, new CopyOfRule());
            digester
                    .addRule(m_pattern + EL_VALUE, new BeanPropertyNullOnEmptyStringRule("value"));
            final String[] properties = {
                "/description", "/profileName", EL_LABEL
            };
            for (int i = 0; i < properties.length; i++) {
                digester.addBeanPropertySetter(m_pattern + properties[i]);
            }
            addSettingTypes(digester, m_pattern + "/type/");
            digester.addSetNext(m_pattern, "addSetting", ConditionalSettingImpl.class.getName());
        }
    }

    static class BeanPropertyNullOnEmptyStringRule extends BeanPropertySetterRule {
        public BeanPropertyNullOnEmptyStringRule(String property) {
            super(property);
        }

        public void body(String namespace, String name, String text) throws Exception {

            super.body(namespace, name, text);
            if (StringUtils.isEmpty(bodyText)) {
                bodyText = null;
            }
        }
    }

    static class CopyOfRule extends Rule {

        public void begin(String namespace_, String name_, Attributes attributes) {
            String copyOfName = attributes.getValue("copy-of");
            if (copyOfName != null) {
                Setting copyTo = (Setting) getDigester().pop();
                Setting parent = (Setting) getDigester().peek();
                // setting to be copied must be defined in file before setting
                // attempting to copy
                Setting copyOf = parent.getSetting(copyOfName);
                Setting copy = copyOf.copy();
                copy.setName(copyTo.getName());
                
                // copies will explicitly not inherit if/unless
                // i think it's against intuition
                ConditionalSetting s = (ConditionalSetting) copy;
                s.setIf(null);
                s.setUnless(null);
                
                getDigester().push(copy);
            }
        }
    }

    class SettingTypeIdRule extends Rule {
        private String m_id;

        public void end(String namespace_, String name_) {
            if (m_id != null) {
                Setting rootSetting = (Setting) getDigester().peek();
                SettingType type = rootSetting.getType();
                m_types.put(m_id, type);
            }
        }

        public void begin(String namespace_, String name_, Attributes attributes) {
            m_id = attributes.getValue("id");
            String refid = attributes.getValue("refid");
            if (refid != null) {
                SettingType type = (SettingType) m_types.get(refid);
                if (type == null) {
                    throw new IllegalArgumentException("Setting type with id=" + refid
                            + " not found.");
                }
                Setting setting = (Setting) getDigester().peek();
                setting.setType(type);
            }
        }
    }

    class SettingTypeRule extends RuleSetBase {
        private final String m_pattern;

        public SettingTypeRule(String pattern) {
            m_pattern = pattern;
        }

        public void addRuleInstances(Digester digester) {
            digester.addSetNext(m_pattern, "setType", SettingType.class.getName());
            digester.addRule(getParentPattern(), new SettingTypeIdRule());
        }

        String getParentPattern() {
            int slash = m_pattern.lastIndexOf('/');
            return m_pattern.substring(0, slash);
        }

        public String getPattern() {
            return m_pattern;
        }
    }

    class StringSettingRule extends SettingTypeRule {
        public StringSettingRule(String pattern) {
            super(pattern);
        }

        public void addRuleInstances(Digester digester) {
            digester.addObjectCreate(getPattern(), StringSetting.class);
            digester.addSetProperties(getPattern());
            digester.addBeanPropertySetter(getPattern() + "/pattern");
            super.addRuleInstances(digester);
        }
    }

    class IntegerSettingRule extends SettingTypeRule {
        public IntegerSettingRule(String pattern) {
            super(pattern);
        }

        public void addRuleInstances(Digester digester) {
            digester.addObjectCreate(getPattern(), IntegerSetting.class);
            digester.addSetProperties(getPattern());
            super.addRuleInstances(digester);
        }
    }

    class RealSettingRule extends SettingTypeRule {
        public RealSettingRule(String pattern) {
            super(pattern);
        }

        public void addRuleInstances(Digester digester) {
            digester.addObjectCreate(getPattern(), RealSetting.class);
            digester.addSetProperties(getPattern());
            super.addRuleInstances(digester);
        }
    }

    class BooleanSettingRule extends SettingTypeRule {
        public BooleanSettingRule(String pattern) {
            super(pattern);
        }

        public void addRuleInstances(Digester digester) {
            String pattern = getPattern();
            digester.addObjectCreate(pattern, BooleanSetting.class);
            digester.addSetProperties(pattern);
            digester.addBeanPropertySetter(pattern + "/true/value", "trueValue");
            digester.addBeanPropertySetter(pattern + "/false/value", "falseValue");
            super.addRuleInstances(digester);
        }
    }

    class EnumSettingRule extends SettingTypeRule {
        public EnumSettingRule(String pattern) {
            super(pattern);
        }

        public void addRuleInstances(Digester digester) {
            digester.addObjectCreate(getPattern(), EnumSetting.class);
            String option = getPattern() + "/option";
            digester.addCallMethod(option, "addEnum", 2);
            digester.addCallParam(option + EL_VALUE, 0);
            digester.addCallParam(option + EL_LABEL, 1);
            super.addRuleInstances(digester);
        }
    }

    class FileSettingRule extends SettingTypeRule {
        public FileSettingRule(String pattern) {
            super(pattern);
        }

        public void addRuleInstances(Digester digester) {
            digester.addObjectCreate(getPattern(), FileSetting.class);
            digester.addSetProperties(getPattern());
            digester.addSetNestedProperties(getPattern());
            super.addRuleInstances(digester);
        }
    }

    private static class ModelEntityResolver implements EntityResolver {
        private static final String DTD = "setting.dtd";

        private File m_dtd;
        private File m_baseSystemId;               

        ModelEntityResolver(File configDirectory, File baseSystemId) {
            m_dtd = new File(configDirectory, DTD);
            m_baseSystemId = baseSystemId;
        }

        public InputSource resolveEntity(String publicId, String systemId) throws IOException {
            if (publicId != null) {
                if (publicId.startsWith("-//SIPFoundry//sipXconfig//Model specification ")) {
                    return new InputSource(new FileInputStream(m_dtd));
                }
            } else if (systemId != null && m_baseSystemId != null) {
                // LIMITATION: All files loaded as ENTITYies defined as SYSTEM
                // must live in same directory as XML file
                //
                // HACK: Xerces 2.7.0 has a propensity to expand systemId to full path
                // which makes it impossible to determine original relative URI. Tricks to use 
                // systemId on inputsource and using file:// failed. 
                String name = new File(systemId).getName();
                File f = new File(m_baseSystemId, name);
                return new InputSource(new FileInputStream(f));                    
            }
                
            return null;
        }
    }
}
